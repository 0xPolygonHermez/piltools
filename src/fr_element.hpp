#ifndef __FR_ELEMENT__HPP__
#define __FR_ELEMENT__HPP__

#include <goldilocks_base_field.hpp>

namespace pil {

typedef Goldilocks::Element FrElement;
inline std::string FrToString(FrElement value) { return Goldilocks::toString(value); };

const uint64_t FRELEMENT_K = 12275445934081160404ULL;

inline void getKs(FrElement *ks, int count)
{
    ks[0] = Goldilocks::fromU64(FRELEMENT_K);
    for (int index = 1; index < count; ++index) {
        ks[index] = Goldilocks::mul(ks[index-1], ks[0]);
    }
}

}
#endif