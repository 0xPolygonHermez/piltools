#include "references.hpp"

namespace pil {

References::References (ReferenceType type)
    :type(type)
{
}

const Reference *References::add(const std::string &name, uid_t id, dim_t len)
{
    if (!len) len = 1;
    for (index_t index = 0; index < len; ++ index) {
        index_t rindex = id + index;
        values[rindex] = new Reference(*this, rindex, len, index, type, name);
    }
    return values[id];
}
/*
const Reference *References::add(const std::string &name, const Reference &value)
{

}*/

const Reference *References::get(uid_t id)
{
    return values[id];
}

void References::map(void *data)
{
    evaluations = (FrElement *)data;
}

FrElement References::getEvaluation(uid_t id, omega_t w, index_t index)
{
    std::cout << "getEvaluation(" << id << "," << w << "," << index << ") p=" << evaluations << std::endl;

    uint64_t offset = (uint64_t)w * values.size() + id + index;
    return evaluations[offset];
}

std::string References::getName(uid_t id)
{
    return values[id]->name;
}

}

