#ifndef __BLOCK_HPP__
#define __BLOCK_HPP__

#include <string>
#include <list>
#include <stdint.h>

namespace pil {

class BlockInfo {
    public:
        std::string title;
        uint64_t ms;
};

class Block {
    protected:
        uint64_t startT;
        static uint64_t firstStartT;
        static uint64_t totalTime;
        static std::list<BlockInfo> blocks;
    public:
        Block (const std::string &title);
        ~Block (void);
        static uint64_t getTotalTime (void);
        static void init (void);
};

}

#endif
