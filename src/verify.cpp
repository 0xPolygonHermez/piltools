#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <map>
#include <string>
#include <cstring>
#include <sys/mman.h>
#include <errno.h>
#include "omp.h"
#include <goldilocks_base_field.hpp>
#include "engine.hpp"
#include "tools.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <getopt.h>
void usage ( const std::string &prgname )
{
    std::cout << "usage:" << std::endl;
    std::cout << "\t" << prgname << " [-m] <commitfile> -c <constfile> -p <pil.json> [-v] [-V <verifyFileByOmegas>] [-j <verifyFileByPols>]" << std::endl;
    exit(EXIT_FAILURE);
}

int main ( int argc, char *argv [])
{
    pil::EngineOptions options;

    int opt;
    if (argc > 1 && argv[1][0] != '-') {
        options.commitFilename = argv[1];
    }

    while ((opt = getopt(argc, argv, "p:c:m:vV:j:S:l:s:id:u:")) != -1)
    {
        switch (opt)
        {
            case 'p': // piljson
                options.pilJsonFilename = optarg;
                break;

            case 'c':
                options.constFilename = optarg ;
                break;

            case 'm':
                options.commitFilename = optarg ;
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

            case 'i':
                options.interactive = true;
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

    pil::Engine engine(options);
}
