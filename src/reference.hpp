#ifndef __PIL__REFERENCE__HPP__
#define __PIL__REFERENCE__HPP__

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>

namespace pil {
    class Reference;
    enum class ReferenceType;
}

#include "fr_element.hpp"
#include "references.hpp"
#include "types.hpp"

namespace pil {

enum class ReferenceType {cmP, constP, imP};


class Reference {
    public:
        References &parent;
        uint id;
        uint polDeg;
        uint len;
        uint index;
        bool isArray;
        ReferenceType type;
        std::string name;
        FrElement getEvaluation(omega_t w, index_t index = 0) const;
        const Reference *getIndex(index_t index) const;
        Reference (References &parent, uid_t id, dim_t len, index_t index, ReferenceType type, const std::string &name);
        std::string getName (void) const { return name; };
};

}

#endif