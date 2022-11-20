#ifndef __PIL__DEPENDENCIES__HPP__
#define __PIL__DEPENDENCIES__HPP__

#include <stdint.h>
#include <list>
#include <nlohmann/json.hpp>
#include <goldilocks_base_field.hpp>

#include "types.hpp"

namespace pil {
    class Dependencies;
}

namespace pil {

class Expressions;

class Dependencies {
    public:
        bool add (uid_t expressionId);
        bool onlyAddIfNew(Dependencies &globalDeps, uid_t expressionId);
        bool contains (uid_t expressionId);
        uint merge (Dependencies &deps);
        size_t size (void) const { return expressionIds.size(); };
        uid_t& operator[](int index) { return expressionIds[index]; };
        Dependencies (void):globalDependencies(NULL) {};
        Dependencies (Dependencies &globalDependencies ): globalDependencies(&globalDependencies) {};
        void dump (void) const;
        bool areEvaluated (const Expressions &expressions) const;
    protected:
        Dependencies *globalDependencies;
        std::vector<uid_t> expressionIds;
};

}
#endif