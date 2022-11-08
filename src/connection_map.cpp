#include <sstream>
#include "connection_map.hpp"
#include "fr_element.hpp"
#include "tools.hpp"

namespace pil {

ConnectionMap::ConnectionMap (dim_t n, uint nk, uint factor)
    :nk(nk), factor(factor), n(n)
{
    pow = Tools::u64Log2(n);
    bits = Tools::u64Log2(n * nk * factor);
    hashSize = (0x1ULL << bits);
    mask = hashSize - 1;
    hashTable = NULL;
    ijTable = NULL;
    count = 0;
    generate();
}

ConnectionMap::~ConnectionMap (void)
{
    if (hashTable) delete [] hashTable;
    if (ijTable) delete [] ijTable;
}

void ConnectionMap::updatePercent(const std::string &title, uint64_t done, uint64_t total)
{
    std::cout << title << " " << pil::Tools::percentBar(done, total);

    if (done == total) {
        std::cout << std::endl << std::flush;
    } else {
        std::cout << "\t\r" << std::flush;
    }
}

void ConnectionMap::generate (void)
{
    auto startT = Tools::startCrono();
    FrElement ks[nk];
    ks[0] = Goldilocks::one();
    getKs(ks+1, nk-1);

    const FrElement wi = Goldilocks::w(pow);
    uint64_t *elements = new uint64_t[n];

    const std::string calculatingTitle = "Calculating w ... ";
    FrElement w = Goldilocks::one();
    for (dim_t i = 0; i < n; ++i) {
        uint64_t u64 = Goldilocks::toU64(w);
        elements[i] = u64;
        w = Goldilocks::mul(w, wi);
        if (i % 10000 == 0) {
            updatePercent(calculatingTitle, i, n);
        }
    }
    updatePercent(calculatingTitle, n, n);

    hashTable = new uint64_t[hashSize];
    ijTable = new uint64_t[hashSize];
    memset(hashTable, 0xFF, sizeof(uint64_t)*hashSize);

    uint64_t collision = 0;
    uint64_t maxCost = 0;

    const uint64_t total = n * nk;
    const std::string PreparingTitle = "Preparing ConnectionMap ...";
    for (dim_t j = 0; j < nk; ++j) {
        updatePercent(PreparingTitle, j*n, total);
        for (dim_t i = 0; i < n; ++i) {
            uint64_t value = j == 0 ? elements[i] : Goldilocks::toU64(Goldilocks::mul(ks[j], ((FrElement *)elements)[i]));
            uint64_t key = hash(value);
            uint64_t cost = 1;
            while (hashTable[key] != 0xFFFFFFFFFFFFFFFFULL) {
                if (hashTable[key] == value) {
                    std::stringstream ss;
                    ss << "Connection map error: duplicated value " << value << " on i:" << i << " j:" << j << " and on i:" << (ijTable[key] & 0xFFFFFFFF) << " j:" << (ijTable[key] >> 32);
                    throw std::runtime_error(ss.str());
                }
                key = hash(value, cost);
                ++cost;
                ++collision;
            }
            if (cost > maxCost) maxCost = cost;
            if (i % 10000 == 0) {
                updatePercent(PreparingTitle, j*nk+i, total);
            }
            hashTable[key] = value;
            ijTable[key] = j << 32 | i;
            ++count;
        }
    }
    updatePercent(PreparingTitle, n*nk, total);

    Tools::endCronoAndShowIt(startT);
    // sizeof(uint64) * 2 * 1024 * 2024 = 3 + 1 + 10 + 10 = 24
    std::cout << "SIZE(MB):" << (hashSize >> 24) << " Collisions:" << collision << " MaxCost:" << maxCost << " Avg.Cost:" << (((double)nk * n) + collision) / ((double)nk * n) << std::endl;


#ifdef __CONNECTION_MAP_VERIFY__
    std::cout << "verifying ....." << std::endl;
    for (dim_t j = 0; j < nk; ++j) {
        for (dim_t i = 0; i < n; ++i) {
            uint64_t value = j == 0 ? elements[i] : Goldilocks::toU64(Goldilocks::mul(ks[j], ((FrElement *)elements)[i]));
            uint64_t res = get(value);
            if (res == NONE) {
                std::cout << "UPS!! not found value:" << value << std::endl;
                exit(1);
            }
            uint64_t _i = res & 0xFFFFFFFF;
            uint64_t _j = res >> 32;
            if (_i != i || j != _j) {
                std::cout << "UPS!! i:" << i << " _i:" << _i << "   j:" << j << " _j:" << _j << std::endl;
                exit(1);
            }
            res = get(value+1);
            if (res != NONE) {
                std::cout << "UPS!! found unexpected value:" << value+1 << std::endl;
                exit(1);
            }
            res = get(value-1);
            if (res != NONE) {
                std::cout << "UPS!! found unexpected value:" << value-1 << std::endl;
                exit(1);
            }
        }
    }
#endif

    delete [] elements;
}

uint64_t ConnectionMap::get (uint64_t value)
{
    uint64_t key = hash(value);
    uint64_t cost = 1;
    while (hashTable[key] != NONE && cost < count)  {
        if (hashTable[key] == value) {
            return ijTable[key];
        }
        key = hash(value, cost);
        ++cost;
    }
    return NONE;
}

uint64_t ConnectionMap::hash(uint64_t value, uint deep)
{
    if (deep) {
        uint64_t offset = ((value >> (bits + deep * 2)) & 0x7) * 7;
        if (!offset) offset = (1 << deep) + deep;
        return (value + deep) & mask;
    }
    return value & mask;
}

}