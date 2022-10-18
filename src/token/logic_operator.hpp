#ifndef __TOKEN__LOGIC_OPERATOR__HPP__
#define __TOKEN__LOGIC_OPERATOR__HPP__

#include <string>
#include "symbol.hpp"
namespace pil::token {

class LogicOperator: public Symbol {
    public:
        LogicOperator (const std::string &content):Symbol(content) {};
        static Token *generate (const std::string &content) { return new LogicOperator(content); };
};

}

#endif