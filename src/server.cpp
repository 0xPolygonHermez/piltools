#include "hv.h"
#include "hssl.h"
#include "hmain.h"
#include "iniparser.h"

#include "HttpServer.h"
#include "hasync.h"     // import hv::async

#include "engine.hpp"
#include "service.hpp"

hv::HttpServer  g_http_server;
hv::HttpService g_http_service;

static int  parse_confile(const char* confile);

// short options
static const char options[] = "hvc:ts:dp:";
// long options
static const option_t long_options[] = {
    {'h', "help",       NO_ARGUMENT},
    {'v', "version",    NO_ARGUMENT},
    {'c', "confile",    REQUIRED_ARGUMENT},
    {'t', "test",       NO_ARGUMENT},
    {'s', "signal",     REQUIRED_ARGUMENT},
    {'d', "daemon",     NO_ARGUMENT},
    {'p', "port",       REQUIRED_ARGUMENT}
};
static const char detail_options[] = R"(
  -h|--help                 Print this information
  -v|--version              Print version
  -c|--confile <confile>    Set configure file, default etc/{program}.conf
  -p|--port <port>          Set listen port
)";


int parse_confile(const char* confile) {
    IniParser ini;
    int ret = ini.LoadFromFile(confile);
    if (ret != 0) {
        printf("Load confile [%s] failed: %d\n", confile, ret);
        exit(-40);
    }

    // logfile
    std::string str = ini.GetValue("logfile");
    if (!str.empty()) {
        strncpy(g_main_ctx.logfile, str.c_str(), sizeof(g_main_ctx.logfile));
    }
    hlog_set_file(g_main_ctx.logfile);
    // loglevel
    str = ini.GetValue("loglevel");
    if (!str.empty()) {
        hlog_set_level_by_str(str.c_str());
    }
    // log_filesize
    str = ini.GetValue("log_filesize");
    if (!str.empty()) {
        hlog_set_max_filesize_by_str(str.c_str());
    }
    // log_remain_days
    str = ini.GetValue("log_remain_days");
    if (!str.empty()) {
        hlog_set_remain_days(atoi(str.c_str()));
    }
    // log_fsync
    str = ini.GetValue("log_fsync");
    if (!str.empty()) {
        logger_enable_fsync(hlog, hv_getboolean(str.c_str()));
    }
    hlogi("%s version: %s", g_main_ctx.program_name, hv_compile_version());
    hlog_fsync();

    // worker_processes
    int worker_processes = 0;
#ifdef DEBUG
    // Disable multi-processes mode for debugging
    worker_processes = 0;
#else
    str = ini.GetValue("worker_processes");
    if (str.size() != 0) {
        if (strcmp(str.c_str(), "auto") == 0) {
            worker_processes = get_ncpu();
            hlogd("worker_processes=ncpu=%d", worker_processes);
        }
        else {
            worker_processes = atoi(str.c_str());
        }
    }
#endif
    g_http_server.worker_processes = LIMIT(0, worker_processes, MAXNUM_WORKER_PROCESSES);
    // worker_threads
    int worker_threads = 0;
    str = ini.GetValue("worker_threads");
    if (str.size() != 0) {
        if (strcmp(str.c_str(), "auto") == 0) {
            worker_threads = get_ncpu();
            hlogd("worker_threads=ncpu=%d", worker_threads);
        }
        else {
            worker_threads = atoi(str.c_str());
        }
    }
    g_http_server.worker_threads = LIMIT(0, worker_threads, 64);

    // worker_connections
    str = ini.GetValue("worker_connections");
    if (str.size() != 0) {
        g_http_server.worker_connections = atoi(str.c_str());
    }

    // http_port
    int port = 0;
    const char* szPort = get_arg("p");
    if (szPort) {
        port = atoi(szPort);
    }
    if (port == 0) {
        port = ini.Get<int>("port");
    }
    if (port == 0) {
        port = ini.Get<int>("http_port");
    }
    g_http_server.port = port;
    // https_port
    if (HV_WITH_SSL) {
        g_http_server.https_port = ini.Get<int>("https_port");
    }
    if (g_http_server.port == 0 && g_http_server.https_port == 0) {
        printf("Please config listen port!\n");
        exit(-10);
    }

    // base_url
    str = ini.GetValue("base_url");
    if (str.size() != 0) {
        g_http_service.base_url = str;
    }
    // document_root
    str = ini.GetValue("document_root");
    if (str.size() != 0) {
        g_http_service.document_root = str;
    }
    // home_page
    str = ini.GetValue("home_page");
    if (str.size() != 0) {
        g_http_service.home_page = str;
    }
    // error_page
    str = ini.GetValue("error_page");
    if (str.size() != 0) {
        g_http_service.error_page = str;
    }
    // index_of
    str = ini.GetValue("index_of");
    if (str.size() != 0) {
        g_http_service.index_of = str;
    }
    // limit_rate
    str = ini.GetValue("limit_rate");
    if (str.size() != 0) {
        g_http_service.limit_rate = atoi(str.c_str());
    }
    // ssl
    if (g_http_server.https_port > 0) {
        std::string crt_file = ini.GetValue("ssl_certificate");
        std::string key_file = ini.GetValue("ssl_privatekey");
        std::string ca_file = ini.GetValue("ssl_ca_certificate");
        hlogi("SSL backend is %s", hssl_backend());
        hssl_ctx_init_param_t param;
        memset(&param, 0, sizeof(param));
        param.crt_file = crt_file.c_str();
        param.key_file = key_file.c_str();
        param.ca_file = ca_file.c_str();
        param.endpoint = HSSL_SERVER;
        if (hssl_ctx_init(&param) == NULL) {
            hloge("SSL certificate verify failed!");
            exit(0);
        }
        else {
            hlogi("SSL certificate verify ok!");
        }
    }

    hlogi("parse_confile('%s') OK", confile);
    return 0;
}

#ifndef __CMDLINE_VERSION__

static void usage ( const std::string &prgname )
{
    std::cout << "usage:" << std::endl;
    std::cout << "\t" << prgname << " [-m] <commitfile> -c <constfile> -p <pil.json> [-v] [-V <verifyFileByOmegas>] [-j <verifyFileByPols>]" << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int ret = 0;
    // g_main_ctx
    main_ctx_init(argc, argv);

    pil::EngineOptions options;

    int opt;
    if (argc > 1 && argv[1][0] != '-') {
        options.commitFilename = argv[1];
    }

    options.checkPlookups = false;
    options.checkPermutations = false;
    options.checkConnections = false;
    options.checkIdentities = false;

    while ((opt = getopt(argc, argv, "p:c:m:vV:j:S:l:s:d:C:u:o")) != -1)
    {
        switch (opt)
        {
            case 'p': // piljson
                options.pilJsonFilename = optarg;
                break;

            case 'c':
                options.constFilename = optarg;
                break;

            case 'o':
                options.overwrite = true;
                break;

            case 'C':
                strncpy(g_main_ctx.confile, optarg, sizeof(g_main_ctx.confile));
                break;

            case 'm':
                options.commitFilename = optarg;
                break;

            case 'v':
                options.verbose = true;
                break;

            case 'V':
                options.expressionsVerifyFilename = optarg;
                options.expressionsVerifyFileMode = pil::EvaluationMapMode::BY_OMEGAS;
                break;

            case 'j':
                options.expressionsVerifyFilename = optarg;
                options.expressionsVerifyFileMode = pil::EvaluationMapMode::BY_POLS;
                break;

            case 'S':
                options.sourcePath = optarg;
                break;

            case 'l':
                options.loadExpressions = true;
                options.expressionsFilename = optarg;
                break;

            case 's':
                options.saveExpressions = true;
                options.expressionsFilename = optarg;
                break;

            case 'u':
                options.publicsFilename = optarg;
                break;

            case 'd':
            {
                const bool disableAll = (strcasecmp(optarg, "ALL") == 0);
                options.checkPlookups = !disableAll && strchr(optarg, 'O') == NULL;
                options.checkPermutations = !disableAll && strchr(optarg, 'E') == NULL;
                options.checkConnections = !disableAll && strchr(optarg, 'C') == NULL;
                options.checkIdentities = !disableAll && strchr(optarg, 'I') == NULL;
                options.calculateExpressions = !disableAll && strchr(optarg, 'X') == NULL;
                break;
            }

            default: /* '?' */
                usage(argv[0]);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "commit file must be specified\n");
        usage(argv[0]);
    }

    parse_confile(g_main_ctx.confile);

    // http_server

    pil::Engine engine(options);
    pil::Service (engine, g_http_service);

    g_http_server.registerHttpService(&g_http_service);
    g_http_server.run();
    return ret;
}
#endif