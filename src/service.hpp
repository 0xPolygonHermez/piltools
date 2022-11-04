#ifndef __SERVICE__HPP__
#define __SERVICE__HPP__

#include <string>
#include <vector>
#include <regex>

#include <nlohmann/json.hpp>

#include "engine.hpp"
#include "types.hpp"

#include "HttpService.h"

using nlohmann::json;

namespace pil {

class Service {
    public:
        Service (Engine &engine, hv::HttpService& router);
    protected:
        Engine &engine;

        class QueryOptions {
            public:
                uint from;
                uint count;
                uint before;
                uint after;
                int skip;
                bool nozeros;
                bool changes;
                bool compact;
                bool hex;
                std::string exportTo;
                std::string trigger;
                std::string filter;
        };

        class FilterInfo {
            public:
                FrElement value;
                const Reference *reference;
                omega_t nextW;
                bool active;
        };

        int query (const HttpContextPtr& ctx);
        int pols (const HttpContextPtr& ctx);
        int namespaces (const HttpContextPtr& ctx);

        QueryOptions parseOptions (const HttpContextPtr& ctx);
        omega_t getTriggeredOmega (const QueryOptions &options, omega_t w);
        omega_t filterSetup (const QueryOptions &options, omega_t w, FilterInfo &filter);
        omega_t filterRows (const QueryOptions &options, omega_t w, FilterInfo &filter);
        std::string jsonQueryToTextFormat (json &j, bool numbersAsString = true, char separator = ',');
        std::string jsonQueryToCsv (json &j);
        std::string jsonQueryToTxt (json &j);
        int generateQueryOutput (const HttpContextPtr& ctx, const QueryOptions &options, json &j, json &jValues, const std::list<std::string> &pols);
        int responseStatus(const HttpContextPtr& ctx, int code = 200, const char* message = NULL);
        int responseStatus(HttpResponse* resp, int code = 200, const char* message = NULL);
        int responseStatus(const HttpResponseWriterPtr& writer, int code = 200, const char* message = NULL);

        static int preProcessor(HttpRequest* req, HttpResponse* resp);
        static int postProcessor(HttpRequest* req, HttpResponse* resp);

        std::string getNamespace (const std::string polname);
        std::list<std::string> filterPols (const HttpContextPtr& ctx, const std::list<std::string> &polnames);

};

}

#endif