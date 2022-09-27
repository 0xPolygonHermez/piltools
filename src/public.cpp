#include "public.hpp"

namespace pil {

void PublicValues::add(int id, const std::string &name, FrElement value)
{
    if (id >= size) {
        uint64_t previousSize = size;
        while (id >= size) {
            size += 128;
        }
        if (values) {
            values = (PublicValue *)realloc(values, size * sizeof(PublicValue));
        } else {
            values = (PublicValue *)malloc(size * sizeof(PublicValue));
        }
        if (!values) {
            throw std::runtime_error("Error on malloc/realloc " +std::to_string(size)+ " public values");
        }
        memset(values + previousSize, 0, (size - previousSize) * sizeof(PublicValue));
    }
    ++count;
    strncpy(values[id].name, name.c_str(), sizeof(values[id].name));
    values[id].value = value;
    values[id].id = id;
}
}