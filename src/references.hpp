#ifndef __PIL__REFERENCES__HPP__
#define __PIL__REFERENCES__HPP__

#include <nlohmann/json.hpp>
#include "fr_element.hpp"

namespace pil {
    class References;
}

#include "reference.hpp"

namespace pil {

class References {
    public:
        const Reference *add(const std::string &name, const Reference &value);
        const Reference *get(uint64_t id);
        void map(void *data);
        FrElement getEvaluation(uint64_t id, uint64_t w);
        std::string getName(uint64_t id);
    protected:
        int count = 0;
        int size = 0;
        Reference *values = NULL;
        FrElement *evaluations;
};

}

#endif