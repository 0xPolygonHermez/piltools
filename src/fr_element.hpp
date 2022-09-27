#ifndef __FR_ELEMENT__HPP__
#define __FR_ELEMENT__HPP__

#include <goldilocks_base_field.hpp>

typedef Goldilocks::Element FrElement;
inline std::string FrToString(FrElement value) { return Goldilocks::toString(value); };

#endif