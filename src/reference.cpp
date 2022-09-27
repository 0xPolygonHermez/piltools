#include "reference.hpp"

namespace pil {

FrElement Reference::getEvaluation(uint64_t w) const
{
    return parent->getEvaluation(id, w);
}

}

