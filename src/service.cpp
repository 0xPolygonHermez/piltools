#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <regex>

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


std::regex Service::genRegExpr (const std::string &pattern)
{
    std::string rex = Tools::replaceAll(Tools::replaceAll(pattern, ".", "\\."), "*", ".*");

    std::stringstream ss(rex);
    std::string token;

    if (pattern.find(',') != std::string::npos) {
        std::string res = "";
        while (std::getline(ss, token, ',')) {
            if (!res.empty()) res += "|";
            res += token;
        }
        rex = "("+res+")";
    }
    rex = "(^" + rex + "$)";
    // std::cout << "PATTERN("<< pattern << ") ==> " << rex << std::endl;
    return std::regex(rex, std::regex_constants::icase);
}

void Service::filterPols (const HttpContextPtr& ctx, std::list<std::string> &polnames)
{
    auto it = polnames.begin();
    bool filterNamespaces = false;
    bool filterNames = false;
    std::string namespacePattern;
    std::string namePattern;

    const auto nsParam = ctx->param("namespace", "*");
    if (!nsParam.empty() && nsParam != "*") {
        filterNamespaces = true;
        namespacePattern = ctx->param("namespace");
    }

    const auto nameParam = ctx->param("name", "*");
    if (!nameParam.empty() && nameParam != "*") {
        filterNames = true;
        namePattern = ctx->param("name");
    }

    if (!filterNamespaces && !filterNames) {
        return;
    }

    std::regex namespaceRegex = genRegExpr(namespacePattern);
    std::regex nameRegex = genRegExpr(namePattern);

    while (it != polnames.end()) {
        auto ns = getNamespace(*it);
        if ((filterNamespaces && !std::regex_search(ns, namespaceRegex)) ||
            (filterNames && !std::regex_search(*it, nameRegex))) {
            it = polnames.erase(it);
            continue;
        }
        ++it;
    }
}
/*
Reference *Service::getReference(const std::string &name)
{
    try {

    }
    catch (const std::exception &e) {
        responseStatus(ctx->response.get(), 500, e.what());
    }
    return NULL;
}
*/
int Service::query(const HttpContextPtr& ctx)
{
    try {
        json j;
        uint count = strtoull(ctx->param("count", "10").c_str(), NULL, 10);
        if (count > 200) count = 200;
        omega_t w = strtoull(ctx->param("from", "0").c_str(), NULL, 10);
        bool nozeros = ctx->param("nozeros", "false") != "false";
        bool changes = ctx->param("changes", "false") != "false";
        bool compact = ctx->param("compact", "false") != "false";
        bool hex = ctx->param("hex", "false") != "false";
        uint before = strtoull(ctx->param("before", "0").c_str(), NULL, 10);
        uint after = strtoull(ctx->param("after", "0").c_str(), NULL, 10);
        int skip = strtoull(ctx->param("skip", "0").c_str(), NULL, 10);
        std::string trigger = ctx->param("trigger");
        std::string filter = ctx->param("filter");

        if (compact) {
            changes = false;
            nozeros = false;
            hex = false;
        }

        std::list<std::string> pols;
        engine.listReferences(pols);
        filterPols(ctx, pols);

        if (trigger != "") {
            auto pos = trigger.find(':');
            std::string polname = trigger.substr(0, pos);
            FrElement value = Goldilocks::fromString(trigger.substr(pos + 1), 10);
            auto ref = engine.getDirectReference(polname);
            while (skip >= 0) {
                while (w < engine.n && !Goldilocks::equal(ref->getEvaluation(w), value)) {
                    ++w;
                }
                --skip;
            }
            w -= before;
        }

        FrElement filterValue;
        const Reference *filterRef = NULL;
        if (filter != "") {
            auto pos = filter.find(':');
            std::string polname = filter.substr(0, pos);
            filterValue = Goldilocks::fromString(filter.substr(pos + 1), 10);
            filterRef = engine.getDirectReference(polname);
            while (w < engine.n && !Goldilocks::equal(filterRef->getEvaluation(w), filterValue)) {
                ++w;
            }
            w -= before;
        }
        omega_t nextFilterW = w;

        std::list<const Reference *> references;
        for (auto it = pols.begin(); it != pols.end(); ++it) {
            references.push_back(engine.getDirectReference(*it));
        }
        json jValues = {};
        uint64_t evaluations[pols.size()];
        for (uint row = 0; row < count; ++row) {
            if (w >= engine.n) break;
            if (filterRef && nextFilterW == w) {
                while (w < engine.n && !Goldilocks::equal(filterRef->getEvaluation(w), filterValue)) {
                    ++w;
                }
                w -= before;
                nextFilterW = w + after + 1;
            }
            json jEvaluations;
            if (compact) {
                jEvaluations.push_back(w);
            } else {
                jEvaluations["!w!"] = w;
            }
            auto itRef = references.begin();
            uint index = 0;
            for (auto it = pols.begin(); it != pols.end(); ++it) {
                uint64_t value = Goldilocks::toU64((*itRef)->getEvaluation(w));
                if ((value || !nozeros) && (!row || !changes || value != evaluations[index])) {
                    if (hex) {
                        char tmp[64];
                        snprintf(tmp, sizeof(tmp), "0x%0lX", value);
                        jEvaluations[*it] = tmp;
                    } else if (compact) {
                        jEvaluations.push_back(value);
                    } else {
                        jEvaluations[*it] = value;
                    }
                }
                ++itRef;
                evaluations[index++] = value;
            }
            jValues.push_back(jEvaluations);
            ++w;
        }
        if (compact) {
            j["titles"] = {"w"};
            for (auto it = pols.begin(); it != pols.end(); ++it) {
                j["titles"].push_back(*it);
            }
        }
        j["values"] = jValues;
        return ctx->sendJson(j);
    }
    catch (const std::exception &e) {
        responseStatus(ctx->response.get(), 500, e.what());
    }
    ctx->send();
    return 500;
}

int Service::pols(const HttpContextPtr& ctx)
{
    json j;
    std::list<std::string> polnames;
    engine.listReferences(polnames);
    filterPols(ctx, polnames);
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