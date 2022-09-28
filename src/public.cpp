#include "public.hpp"

namespace pil {

void PublicValues::add(uid_t id, const std::string &name, FrElement value)
{
    PublicValue pvalue;
    pvalue.name = name;
    pvalue.value = value;
    pvalue.id = id;
    values[id] = pvalue;
}
}