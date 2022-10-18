#include "symbol.hpp"

#include "logic_operator.hpp"
#include "arith_operator.hpp"

namespace pil::token {

Symbol *Symbol::generate (const std::string &content) {
    if (Token::indexInList("&& || ! == != < > >= <=", content)) {
        return new LogicOperator(content);
    }
    if (Token::indexInList("+ - * >> <<", content)) {
        return new ArithOperator(content);
    }
    return new Symbol(content);
};

}