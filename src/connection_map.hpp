#ifndef __PIL__CONNECTION_MAP__HPP__
#define __PIL__CONNECTION_MAP__HPP__

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "types.hpp"

namespace pil {

class ConnectionMap {
    public:
        const uint64_t NONE = 0xFFFFFFFFFFFFFFFF;

        ConnectionMap (dim_t n, uint nk, uint factor = 8);
        ~ConnectionMap (void);
        uint64_t hash (uint64_t value, uint deep = 0);
        size_t size (void) { return count; };
        uint64_t get (uint64_t value);
        uint64_t ij2i (uint64_t ij) { return ij & 0xFFFFFFFF; };
        uint64_t ij2j (uint64_t ij) { return ij >> 32; };
    protected:
        uint nk;
        uint bits;
        uint pow;
        uint factor;
        uint hashSize;
        uint64_t n;
        uint64_t mask;
        uint64_t *hashTable;
        uint64_t *ijTable;
        uint64_t count;
        void generate (void);
        void updatePercent(const std::string &title, uint64_t done, uint64_t total);
};

}
#endif