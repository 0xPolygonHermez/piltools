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

#include <nlohmann/json.hpp>
using json = nlohmann::json;


/*
// /home/ubuntu//zkevm-proverjs/build/v0.4.0.0-rc.1-basic/zkevm.expr
const std::string basePath = "/home/ubuntu/zkevm-proverjs/build/v0.4.0.0-rc.1-basic/";
const std::string pilJsonFilename = basePath + "basic_main.pil.json";
const std::string constFilename = basePath + "zkevm.const";
const std::string commitFilename = basePath + "zkevm.commit";
*/

const std::string basePath = "/home/ubuntu/zkevm-proverjs/build/v0.3.0.0-rc.1/";
const std::string pilJsonFilename = basePath + "main.pil.json";
const std::string constFilename = basePath + "zkevm.const";
const std::string commitFilename = basePath + "zkevm.commit";
/*
const std::string pilJsonFilename = "../../data/v0.3.0.0-rc.1/main.pil.json";
const std::string constFilename = "../../data/v0.3.0.0-rc.1/zkevm.const";
const std::string commitFilename = "../../data/v0.3.0.0-rc.1/zkevm.commit";

const std::string pilJsonFilename = "../../data/v0.4.0.0-rc.1-basic/basic_main.pil.json";
const std::string constFilename = "../../data/v0.4.0.0-rc.1-basic/zkevm.const";
const std::string commitFilename = "../../data/v0.4.0.0-rc.1-basic/zkevm.commit";
*/

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


void calculateFieldContants ( Goldilocks &fr, uint32_t n )
{
    /*
    u_int32_t domainPow = NTT_Goldilocks::log2(n);

        mpz_t m_qm1d2;
        mpz_t m_q;
        mpz_t m_nqr;
        mpz_t m_aux;
        mpz_init(m_qm1d2);
        mpz_init(m_q);
        mpz_init(m_nqr);
        mpz_init(m_aux);

        u_int64_t negone = GOLDILOCKS_PRIME - 1;

        mpz_import(m_aux, 1, 1, sizeof(u_int64_t), 0, 0, &negone);
        mpz_add_ui(m_q, m_aux, 1);
        mpz_fdiv_q_2exp(m_qm1d2, m_aux, 1);

        mpz_set_ui(m_nqr, 2);
        mpz_powm(m_aux, m_nqr, m_qm1d2, m_q);
        while (mpz_cmp_ui(m_aux, 1) == 0)
        {
            mpz_add_ui(m_nqr, m_nqr, 1);
            mpz_powm(m_aux, m_nqr, m_qm1d2, m_q);
        }    */
}

int main ( int argc, char *argv [])
{
    // Goldilocks fr;
    // std::cout << "nqr:" << Goldilocks::toString(fr.nqr) << std::endl;
    // std::cout << "W[1]:" << Goldilocks::toString(Goldilocks::W[1]) << std::endl;
    u64Log2(1);
    u64Log2(2);
    u64Log2(10);
    u64Log2(18);
    u64Log2(31);
    u64Log2(32);
    u64Log2(33);
    u64Log2(1023);
    u64Log2(1024);
    u64Log2(1025);
    u64Log2(1UL << 63);
    u64Log2((1UL << 63) + 1);
    u64Log2(0xFFFFFFFFFFFFFFFFULL);
    u64Log2(0);

    pil::Engine engine(pilJsonFilename, constFilename, commitFilename);
    engine.getEvaluation("Main.STEP", 10);
    // engine.getEvaluation("Arith.x1", 12, 3);
    engine.getEvaluation("Main.A0", 12);
    engine.getEvaluation("Main.STEP", 9584);
    // std::cout << pil.dump() << std::endl;
}