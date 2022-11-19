#include "public.hpp"

namespace pil {

void PublicValues::add(uid_t id, const std::string &name, FrElement value)
{
    std::cout << "PublicValues::add(" << id << "," << name << "," << Goldilocks::toString(value) << ")" << std::endl;
    PublicValue pvalue;
    pvalue.name = name;
    pvalue.value = value;
    pvalue.id = id;
    values[id] = pvalue;
}
}