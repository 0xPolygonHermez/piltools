
#include <unordered_map>
#include <string>
#include <regex>
#include <stdint.h>
#include <sstream>

#include "fr_element.hpp"

#include "service.hpp"

#include "hthread.h"
#include "hasync.h"     // import hv::async
#include "requests.h"   // import requests::async

using nlohmann::json;

namespace pil {

Service::Service (Engine &engine, hv::HttpService& router)
    :engine(engine)
{
    router.preprocessor = preProcessor;
    router.postprocessor = postProcessor;

    std::cout << "document_root: " << router.document_root << std::endl;
    router.GET("/query", [this](auto ctx){ return query(ctx); });
    router.GET("/namespaces", [this](auto ctx){ return namespaces(ctx); });
    router.GET("/pols", [this](auto ctx){ return pols(ctx); });
}

std::string Service::getNamespace (const std::string polname)
{
    auto pos = polname.find('.');
    if (pos == std::string::npos) return "";

    return polname.substr(0, pos);
}

std::list<std::string> Service::filterPols (const HttpContextPtr& ctx, const std::list<std::string> &polnames)
{
    bool filterNames = false;
    std::string namePattern;

    const auto nameParam = ctx->param("name", "*");
    if (!nameParam.empty() && nameParam != "*") {
        filterNames = true;
        namePattern = ctx->param("name");
    }

    if (!filterNames) {
        return polnames;
    }

    std::stringstream ss(namePattern);
    std::string pattern;
    std::list<std::string> result;
    auto included = new bool[polnames.size()]();
    while (std::getline(ss, pattern, ',')) {
        std::regex rex = std::regex("(^" + Tools::replaceAll(Tools::replaceAll(pattern, ".", "\\."), "*", ".*") + "$)", std::regex_constants::icase);
        uint index = 0;
        for (auto it = polnames.cbegin(); it != polnames.cend(); ++it, ++index) {
            if (included[index]) continue;
            if (!std::regex_search(*it, rex)) continue;
            included[index] = true;
            result.push_back(*it);
        }
    }
    delete [] included;
    return result;
}


Service::QueryOptions Service::parseOptions (const HttpContextPtr& ctx)
{
    QueryOptions options;
    options.from = strtoull(ctx->param("from", "0").c_str(), NULL, 10);
    options.nozeros = ctx->param("nozeros", "false") != "false";
    options.changes = ctx->param("changes", "false") != "false";
    options.compact = ctx->param("compact", "false") != "false";
    options.hex = ctx->param("hex", "false") != "false";
    options.before = strtoull(ctx->param("before", "0").c_str(), NULL, 10);
    options.after = strtoull(ctx->param("after", "0").c_str(), NULL, 10);
    options.skip = strtoull(ctx->param("skip", "0").c_str(), NULL, 10);
    options.trigger = ctx->param("trigger");
    options.filter = ctx->param("filter");

    options.exportTo = ctx->param("export", "");
    std::transform(options.exportTo.begin(), options.exportTo.end(), options.exportTo.begin(),
            [](unsigned char c){ return std::tolower(c); });

    options.count = strtoull(ctx->param("count", "10").c_str(), NULL, 10);
    if (options.exportTo.empty() && options.count > 200) options.count = 200;

    if (options.compact) {
        options.changes = false;
        options.nozeros = false;
        options.hex = false;
    }
    return options;
}

omega_t Service::getTriggeredOmega (const QueryOptions &options, omega_t w)
{
    auto pos = options.trigger.find(':');
    std::string polname = options.trigger.substr(0, pos);
    FrElement value = Goldilocks::fromString(options.trigger.substr(pos + 1), 10);
    auto ref = engine.getDirectReference(polname);
    for (uint index = 0; index <= options.skip; ++index) {
        while (w < engine.n && !Goldilocks::equal(ref->getEvaluation(w), value)) {
            ++w;
        }
    }
    w -= options.before;
    return w;
}

omega_t Service::filterSetup (const QueryOptions &options, omega_t w, FilterInfo &filter)
{
    if (!(filter.active = (options.filter != ""))) {
        return w;
    }

    std::stringstream fss(options.filter);
    while (fss.good()) {
        std::string cond;
        getline(fss, cond, ',');

        FilterInfoCond fcond;
        auto pos = cond.find(':');
        fcond.value =  Goldilocks::fromString(cond.substr(pos + 1), 10);
        fcond.reference = engine.getDirectReference(cond.substr(0, pos));
        filter.conds.push_back(fcond);
    }
    while (w < engine.n) {
        auto it = filter.conds.begin();
        while  (it != filter.conds.end() && Goldilocks::equal(it->reference->getEvaluation(w), it->value)) {
            ++it;
        }
        if (it == filter.conds.end()) {
            break;
        }
        ++w;
    }
    w -= options.before;
    filter.nextW = w;
    return w;
}

omega_t Service::filterRows (const QueryOptions &options, omega_t w, FilterInfo &filter)
{
    if (!filter.active || !filter.conds.size()) {
        return w;
    }

    while (w < engine.n) {
        auto it = filter.conds.begin();
        while  (it != filter.conds.end() && Goldilocks::equal(it->reference->getEvaluation(w), it->value)) {
            ++it;
        }
        if (it == filter.conds.end()) {
            break;
        }
        ++w;
    }

    w -= options.before;
    filter.nextW = w + options.after + 1;

    return w;
}

int Service::query (const HttpContextPtr& ctx)
{
    try {
        json j;
        auto options = parseOptions(ctx);
        omega_t w = options.from;

        std::list<std::string> pols;
        engine.listReferences(pols);
        pols = filterPols(ctx, pols);

        if (options.trigger != "") {
            w = getTriggeredOmega(options, w);
        }

        FilterInfo filter;
        w = filterSetup(options, w, filter);

        std::list<const Reference *> references;
        for (auto it = pols.begin(); it != pols.end(); ++it) {
            references.push_back(engine.getDirectReference(*it));
        }
        json jValues = {};
        uint64_t evaluations[pols.size()];
        for (uint row = 0; row < options.count; ++row) {
            w = filterRows(options, w, filter);
            if (w >= engine.n) break;
            json jEvaluations;
            if (options.compact) {
                jEvaluations.push_back(std::to_string(w));
            } else {
                jEvaluations["!w!"] = std::to_string(w);
            }
            auto itRef = references.begin();
            uint index = 0;
            for (auto it = pols.begin(); it != pols.end(); ++it) {
                uint64_t value = Goldilocks::toU64((*itRef)->getEvaluation(w));
                if ((value || !options.nozeros) && (!row || !options.changes || value != evaluations[index])) {
                    // max 20/18 chars
                    char tmp[32];
                    if (options.hex) {
                        snprintf(tmp, sizeof(tmp), "0x%0lX", value);
                        jEvaluations[*it] = tmp;
                    } else {
                        snprintf(tmp, sizeof(tmp), "%lu", value);
                        if (options.compact) {
                            jEvaluations.push_back(tmp);
                        } else {
                            jEvaluations[*it] = tmp;
                        }
                    }
                }
                ++itRef;
                evaluations[index++] = value;
            }
            jValues.push_back(jEvaluations);
            ++w;
        }
        return generateQueryOutput(ctx, options, j, jValues, pols);
    }
    catch (const std::exception &e) {
        responseStatus(ctx->response.get(), 500, e.what());
    }
    ctx->send();
    return 500;
}

int Service::generateQueryOutput (const HttpContextPtr& ctx, const QueryOptions &options, json &j, json &jValues, const std::list<std::string> &pols)
{
    if (options.compact) {
        j["titles"] = {"w"};
        for (auto it = pols.begin(); it != pols.end(); ++it) {
            j["titles"].push_back(*it);
        }
    }
    j["values"] = jValues;
    if (options.exportTo != "") {
        std::stringstream sfilename;
        auto t = std::time(nullptr);
        auto tm = *std::gmtime(&t);
        sfilename << std::put_time(&tm, "%Y%m%d_%H%M%S_") << options.from << "_" << options.count << "." << options.exportTo;
        if (options.exportTo == "csv") {
            std::string content = jsonQueryToCsv(j);
            ctx->response->headers["Content-disposition"] = "attachment;filename=" + sfilename.str();
            return ctx->send(content, TEXT_CSV);
        }
        if (options.exportTo == "txt") {
            ctx->response->headers["Content-disposition"] = "attachment;filename=" + sfilename.str();
            std::string content = jsonQueryToTxt(j);
            return ctx->send(content, TEXT_PLAIN);
        }
    }
    return ctx->sendJson(j);
}

std::string Service::jsonQueryToCsv (json &j)
{
    return jsonQueryToTextFormat(j, true, ',');
}

std::string Service::jsonQueryToTxt (json &j)
{
    return jsonQueryToTextFormat(j, false, ',');
}

std::string Service::jsonQueryToTextFormat (json &j, bool numbersAsString, char separator)
{
    std::stringstream content;
    const auto titles = j["titles"];
    for (auto it = titles.cbegin(); it != titles.cend(); ++it) {
        if (it != titles.cbegin()) content << separator;
        content << '"' << (*it).get<std::string>() << '"';
    }
    content << "\n";

    const auto rows = j["values"];
    uint irow = 0;
    std::string numberDelimiter = numbersAsString ? "\"":"";
    for (auto row = rows.cbegin(); row != rows.cend(); ++row, ++irow) {
        uint icol = 0;
        for (auto it = (*row).cbegin(); it != (*row).cend(); ++it, ++icol) {
            if (it != (*row).cbegin()) content << separator;
            content << numberDelimiter << (*it).get<std::string>() << numberDelimiter;
        }
        content << "\n";
    }
    return content.str();
}

int Service::pols(const HttpContextPtr& ctx)
{
    json j;
    std::list<std::string> polnames;
    engine.listReferences(polnames);
    j["pols"] = polnames;
    return ctx->sendJson(j);
}

int Service::namespaces(const HttpContextPtr& ctx)
{
    json j;
    j["namespaces"] = engine.namespaces;
    return ctx->sendJson(j);
}


int Service::responseStatus (const HttpContextPtr& ctx, int code, const char* message)
{
    responseStatus(ctx->response.get(), code, message);
    ctx->send();
    return code;
}

int Service::responseStatus (HttpResponse* resp, int code, const char* message)
{
    if (message == NULL) message = http_status_str((enum http_status)code);
    resp->Set("code", code);
    resp->Set("message", message);
    return code;
}

int Service::responseStatus (const HttpResponseWriterPtr& writer, int code, const char* message)
{
    responseStatus(writer->response.get(), code, message);
    writer->End();
    return code;
}

int Service::preProcessor(HttpRequest* req, HttpResponse* resp)
{
    // cors
    resp->headers["Access-Control-Allow-Origin"] = "*";
    if (req->method == HTTP_OPTIONS) {
        resp->headers["Access-Control-Allow-Origin"] = req->GetHeader("Origin", "*");
        resp->headers["Access-Control-Allow-Methods"] = req->GetHeader("Access-Control-Request-Method", "OPTIONS, HEAD, GET, POST, PUT, DELETE, PATCH");
        resp->headers["Access-Control-Allow-Headers"] = req->GetHeader("Access-Control-Request-Headers", "Content-Type");
        return HTTP_STATUS_NO_CONTENT;
    }

    // Deserialize request body to json, form, etc.
    req->ParseBody();

    // Unified setting response Content-Type?
    resp->content_type = APPLICATION_JSON;

    return HTTP_STATUS_UNFINISHED;
}

int Service::postProcessor(HttpRequest* req, HttpResponse* resp)
{
    // printf("%s\n", resp->Dump(true, true).c_str());
    return resp->status_code;
}

}