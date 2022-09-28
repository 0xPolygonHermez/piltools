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

class Dependencies {
    public:
        std::list<depid_t> pending;
        std::list<depid_t> handled;
        bool add (depid_t expressionId);
        size_t size ( void ) const { return pending.size() + handled.size(); };
        Dependencies ( void ) {};
};

}
#endif