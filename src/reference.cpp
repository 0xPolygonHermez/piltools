#include "reference.hpp"

namespace pil {

FrElement Reference::getEvaluation (omega_t w, index_t index) const
{
    return parent.getEvaluation(id, w, index);
}


const Reference *Reference::getIndex (index_t index) const
{
    return ((index == this->index) ? this : parent.get(id + index - this->index));
}

Reference::Reference (References &parent, uid_t id, dim_t len, index_t index, ReferenceType type, const std::string &name)
    :parent(parent), id(id), len(len), index(index), type(type), name(name)
{
    polDeg = 0;
    isArray = len > 1;
/*    if (isArray) {
        this->name = name + "[" + std::to_string(index)+"]";
    }*/
}

}
