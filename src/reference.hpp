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


namespace pil {

enum class ReferenceType {cmP, constP, imP};


class Reference {
    public:
        uint64_t id = 0;
        uint64_t polDeg = 0;
        uint64_t len = 0;
        uint64_t index = 0;
        bool isArray = false;
        ReferenceType type;
        char name[64] = "";
        References *parent;
        FrElement getEvaluation(uint64_t w) const;
};

}

#endif