#ifndef __SERVICE__HPP__
#define __SERVICE__HPP__

#include <string>
#include <vector>
#include <regex>
#include "engine.hpp"
#include "types.hpp"

#include "HttpService.h"

namespace pil {

class Service {
    public:
        Service (Engine &engine, hv::HttpService& router);
    protected:
        Engine &engine;

        int query (const HttpContextPtr& ctx);
        int pols (const HttpContextPtr& ctx);
        int namespaces (const HttpContextPtr& ctx);

        int responseStatus(const HttpContextPtr& ctx, int code = 200, const char* message = NULL);
        int responseStatus(HttpResponse* resp, int code = 200, const char* message = NULL);
        int responseStatus(const HttpResponseWriterPtr& writer, int code = 200, const char* message = NULL);

        static int preProcessor(HttpRequest* req, HttpResponse* resp);
        static int postProcessor(HttpRequest* req, HttpResponse* resp);

        std::string getNamespace (const std::string polname);
        std::regex genRegExpr (const std::string &pattern);
        void filterPols (const HttpContextPtr& ctx, std::list<std::string> &polnames);

};

}

#endif