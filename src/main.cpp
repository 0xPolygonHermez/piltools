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
#include <sys/mman.h>
#include <errno.h>
#include "omp.h"
#include <goldilocks_base_field.hpp>
#include "engine.hpp"
#include "tools.hpp"
#include "parser.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define FULL4

#ifdef BASIC4
// /home/ubuntu//zkevm-proverjs/build/v0.4.0.0-rc.1-basic/zkevm.expr
const std::string basePath = "/home/ubuntu/zkevm-proverjs/build/v0.4.0.0-rc.1-basic/";
const std::string pilJsonFilename = basePath + "basic_main.pil.json";
const std::string constFilename = basePath + "zkevm.const";
const std::string commitFilename = basePath + "zkevm.commit";
#endif

#ifdef FULL3
const std::string basePath = "/home/ubuntu/zkevm-proverjs/build/v0.3.0.0-rc.1/";
const std::string pilJsonFilename = basePath + "main.pil.json";
const std::string constFilename = basePath + "zkevm.const";
const std::string commitFilename = basePath + "zkevm.commit";
#endif

#ifdef FULL4
const std::string basePath = "/home/ubuntu/zkevm-proverjs/build/v0.4.0.0-rc.1/";
const std::string pilJsonFilename = basePath + "main.pil.json";
const std::string constFilename = basePath + "zkevm.const";
const std::string commitFilename = basePath + "zkevm.commit";
#endif
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
    pil::Parser p;


    p.compile("A +   3 === 12 *    2 ");
    p.compile("function(Pols,Pols[4], Main.Pols[4], Main.Clock, Main.A0, DEF,HHJK)");

/*
    int opt;
    if (argc > 1 && argv[1][0] != '-') {
        options.commitFilename = argv[1];
    }

    while ((opt = getopt(argc, argv, "p:c:m:vV:j:S:l:s:")) != -1)
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

            default:
                usage(argv[0]);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "commit file must be specified\n");
        usage(argv[0]);
    }

    pil::Engine engine(options);
*/
     /*

    exit(EXIT_SUCCESS);
    pil::Engine engine({
        pilJsonFilename : pilJsonFilename,
        constFilename : constFilename,
        commitFilename : commitFilename,
        //        commitFilename: basePath + "zkevm.fake.commit",
        //        loadExpressions: true,
        //        saveExpressions: true,
        expressionsFilename : basePath + "zkevm.fake.expr.bypols.bin"
    });
    // engine.getEvaluation("Main.STEP", 10);
    // engine.getEvaluation("Arith.x1", 12, 3);
    // engine.getEvaluation("Main.A0", 12);
    // engine.getEvaluation("Main.STEP", 9584);
    // std::cout << pil.dump() << std::endl;*/
}