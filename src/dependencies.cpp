#include <algorithm>
#include "dependencies.hpp"
#include "expression.hpp"

namespace pil {

bool Dependencies::add(uid_t expressionId)
{
    if (globalDependencies && globalDependencies->contains(expressionId)) {
        return false;
    }
    if (contains(expressionId)) {
        return false;
    }
    expressionIds.push_back(expressionId);
    return true;
}

bool Dependencies::onlyAddIfNew(Dependencies &globalDeps, uid_t expressionId)
{
    if (globalDeps.contains(expressionId)) {
        return false;
    }
    return add(expressionId);
}

bool Dependencies::contains (uid_t expressionId )
{
    return (std::find(expressionIds.begin(), expressionIds.end(), expressionId) != expressionIds.end());
}

uint Dependencies::merge(Dependencies &deps)
{
    uint count = 0;
    for (auto it = deps.expressionIds.begin(); it != deps.expressionIds.end(); ++it) {
        if (std::find(expressionIds.begin(), expressionIds.end(), *it) != expressionIds.end()) {
            continue;
        }
        ++count;
        expressionIds.push_back(*it);
    }
    return count;
}

void Dependencies::dump ( void ) const
{
    std::cout << "DEPS[" << expressionIds.size() << "]";
    for (auto it = expressionIds.begin(); it != expressionIds.end(); ++it) {
        std::cout << " " << *it;
    }
    std::cout << std::endl;
}



}