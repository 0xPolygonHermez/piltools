#ifndef __FR_ELEMENT__HPP__
#define __FR_ELEMENT__HPP__

#include <goldilocks_base_field.hpp>

namespace pil {

typedef Goldilocks::Element FrElement;
inline std::string FrToString(FrElement value) { return Goldilocks::toString(value); };

const uint64_t FRELEMENT_K = 12275445934081160404ULL;
const uint64_t FRELEMENT_W32 = 7277203076849721926ULL;

inline void getKs(FrElement *ks, int count)
{
    ks[0] = Goldilocks::one();
    ks[1] = Goldilocks::fromU64(FRELEMENT_K);
    for (int index = 2; index < count; ++index) {
        ks[index] = Goldilocks::mul(ks[index-1], ks[1]);
    }
}

inline FrElement getRoot(int pow)
{
    FrElement root = Goldilocks::fromU64(FRELEMENT_W32);
    int index = 32;
    while (index > pow) {
        root = Goldilocks::square(root);
        --index;
    }
    return root;
}

}
#endif