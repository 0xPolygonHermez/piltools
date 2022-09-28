#include <algorithm>
#include "dependencies.hpp"
#include "expression.hpp"

namespace pil {

bool Dependencies::add(depid_t expressionId)
{
    if (std::find(pending.begin(), pending.end(), expressionId) != pending.end()) {
        std::cout << "Dependencies::add(1)" << std::endl;
        return false;
    }
    if (std::find(handled.begin(), handled.end(), expressionId) != handled.end()) {
        std::cout << "Dependencies::add(2)" << std::endl;
        return false;
    }
    std::cout << "Dependencies::add(3)" << std::endl;
    pending.push_back(expressionId);
    std::cout << "Dependencies::add(4)" << std::endl;
    return true;
}

}