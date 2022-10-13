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

uint64_t u64Log2 ( uint64_t value )
{
    uint64_t upto = 1;
    uint64_t log = 0;
    while (value > upto) {
        ++log;
        if (upto >= 0x8000000000000000UL) break;
        upto *= 2;
    }
    std::cout << "u64Log(" << value << ")=" << log << std::endl;
    return log;
}

int main ( int argc, char *argv [])
{
    pil::Engine engine({
        pilJsonFilename: pilJsonFilename,
        constFilename: constFilename,
        // commitFilename: commitFilename,
        commitFilename: basePath + "zkevm.fake.commit",
        loadExpressions: true,
//        saveExpressions: true,
        expressionsFilename: basePath + "zkevm.fake.expr.bypols.bin"
    });
    // engine.getEvaluation("Main.STEP", 10);
    // engine.getEvaluation("Arith.x1", 12, 3);
    // engine.getEvaluation("Main.A0", 12);
    engine.getEvaluation("Main.STEP", 9584);
    // std::cout << pil.dump() << std::endl;*/
}