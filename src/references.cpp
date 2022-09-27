#include "references.hpp"

namespace pil {

const Reference *References::add(const std::string &name, const Reference &value)
{
    int len = value.len > 0 ? value.len : 1;
    int maxid = value.id + len - 1;
    if (maxid >= size) {
        uint64_t previousSize = size;
        while (maxid >= size) {
            size += 1000;
        }
        if (values) {
            values = (Reference *)realloc(values, size * sizeof(Reference));
        } else {
            values = (Reference *)malloc(size * sizeof(Reference));
        }
        if (!values) {
            throw std::runtime_error("Error on malloc/realloc " +std::to_string(size)+ " values");
        }
        memset(values + previousSize, 0, (size - previousSize) * sizeof(Reference));
    }
    for (int index = 0; index < len; ++ index) {
        int rindex = value.id + index;
        ++count;
        if (values[rindex].id) {
            std::cout << "ooohh !!! " << values[rindex].name << " by " << name << std::endl;
        }
        values[rindex] = value;
        if (value.isArray) {
            snprintf(values[rindex].name, sizeof(value.name), "%s[%d]", name.c_str(), index);
        } else {
            snprintf(values[rindex].name, sizeof(value.name), "%s", name.c_str());
        }
        values[rindex].index = index;
        values[rindex].id = rindex;
        values[rindex].parent = this;
    }
    return values + value.id;
}
/*
const Reference *References::add(const std::string &name, const Reference &value)
{

}*/

const Reference *References::get(uint64_t id)
{
    return values + id;
}

void References::map(void *data)
{
    evaluations = (FrElement *)data;
}

FrElement References::getEvaluation(uint64_t id, uint64_t w)
{
    return evaluations[ w * count + id];
}

std::string References::getName(uint64_t id)
{
    return values[id].name;
}

}

