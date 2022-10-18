#ifndef __TOKEN__ARITH_OPERATOR__HPP__
#define __TOKEN__ARITH_OPERATOR__HPP__

#include <string>
#include "symbol.hpp"

namespace pil::token {

class ArithOperator: public Symbol {
    public:
        ArithOperator (const std::string &content):Symbol(content) {};
        static Token *generate (const std::string &content) { return new ArithOperator(content); };
};

}

#endif