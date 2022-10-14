#include "block.hpp"
#include "tools.hpp"

namespace pil {

uint64_t Block::firstStartT = 0;
uint64_t Block::totalTime = 0xFFFFFFFFFFFFFFFF;

Block::Block(const std::string &title)
{
    startT = Tools::startCrono();
    if (firstStartT == 0) firstStartT = startT;
    std::cout << "==== " << title << " ====" << std::endl;
}


Block::~Block (void)
{
    uint64_t ms = Tools::endCrono(startT);
    std::cout << "Time(ms): " << ms << std::endl << std::endl;
}

uint64_t Block::getTotalTime ( void )
{
    if (totalTime == 0xFFFFFFFFFFFFFFFF) {
        totalTime = Tools::endCrono(firstStartT);
    }
    return totalTime;
}

void Block::init ( void )
{
    firstStartT = Tools::startCrono();
}

}