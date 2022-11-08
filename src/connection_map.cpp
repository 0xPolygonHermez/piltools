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

void ConnectionMap::updatePercent(uint64_t done)
{
    const uint64_t total = n * nk;
    std::cout << "Preparing ConnectionMap " << pil::Tools::percentBar(done, total);

    if (done == total) {
        std::cout << std::endl << std::flush;
    } else {
        std::cout << "\t\r" << std::flush;
    }
}

void ConnectionMap::generate (void)
{
#ifdef __CONNECTION_MAP_STATISTICS__
    auto startT = Tools::startCrono();
#endif
    FrElement ks[nk];
    ks[0] = Goldilocks::one();
    getKs(ks+1, nk-1);

    const FrElement wi = Goldilocks::w(pow);
    uint64_t *elements = new uint64_t[n];

    FrElement w = Goldilocks::one();
    for (dim_t i = 0; i < n; ++i) {
        uint64_t u64 = Goldilocks::toU64(w);
        elements[i] = u64;
        w = Goldilocks::mul(w, wi);
    }

    hashTable = new uint64_t[hashSize];
    ijTable = new uint64_t[hashSize];
    memset(hashTable, 0xFF, sizeof(uint64_t)*hashSize);

#ifdef __CONNECTION_MAP_STATISTICS__
    uint64_t collision = 0;
    uint64_t maxCost = 0;
#endif
    for (dim_t j = 0; j < nk; ++j) {
        updatePercent(j*nk);
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
#ifdef __CONNECTION_MAP_STATISTICS__
                ++collision;
#endif
            }
#ifdef __CONNECTION_MAP_STATISTICS__
            if (cost > maxCost) maxCost = cost;
#endif
            if (i % 10000 == 0) {
                updatePercent(j*nk+i);
            }
            hashTable[key] = value;
            ijTable[key] = j << 32 | i;
            ++count;
        }
    }
    updatePercent(n*nk);

#ifdef __CONNECTION_MAP_STATISTICS__
    Tools::endCronoAndShowIt(startT);
    std::cout << "SIZE(MB):" << (hashSize >> 16) << " COLLISIONS:" << collision << " MAXCOST:" << maxCost << " AVG.COST" << (((double)nk * n) + collision) / ((double)nk * n) << std::endl;
#endif


#ifdef __CONNECTION_MAP_VERIFY__
    std::cout << "verifying ....." << std::endl;
    for (dim_t j = 0; j < nk; ++j) {
        for (dim_t i = 0; i < n; ++i) {
            uint64_t value = j == 0 ? elements[i] : Goldilocks::toU64(Goldilocks::mul(ks[j], ((FrElement *)elements)[i]));
            uint64_t res = get(value);
            if (res == 0xFFFFFFFFFFFFFFFFULL) {
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
            if (res != 0xFFFFFFFFFFFFFFFFULL) {
                std::cout << "UPS!! found unexpected value:" << value+1 << std::endl;
                exit(1);
            }
            res = get(value-1);
            if (res != 0xFFFFFFFFFFFFFFFFULL) {
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
    while (hashTable[key] != 0xFFFFFFFFFFFFFFFFULL && cost < count)  {
        if (hashTable[key] == value) {
            return ijTable[key];
        }
        key = hash(value, cost);
        ++cost;
    }
    return 0xFFFFFFFFFFFFFFFFULL;
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