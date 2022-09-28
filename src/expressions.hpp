#ifndef __PIL__EXPRESSIONS__HPP__
#define __PIL__EXPRESSIONS__HPP__

namespace pil {
    class Expressions;
}

#include "reference.hpp"
#include "fr_element.hpp"

namespace pil {

class Expressions {
    public:
        const Reference *add(const std::string &name, const Reference &value);
        const Reference *get(uint id) { return values + id; };
        void map(void *data) { evaluations = (FrElement *)data; }
        FrElement getEvaluation(uint id, uint w) { return evaluations[ w * count + id]; }
        std::string getName(uint id) { return values[id].name; }
    protected:
        int count = 0;
        int size = 0;
        Reference *values = NULL;
        FrElement *evaluations;
};

}
#endif