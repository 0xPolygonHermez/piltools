#ifndef __TOKEN__SYMBOL__HPP__
#define __TOKEN__SYMBOL__HPP__

#include <string>
#include "../token.hpp"

namespace pil::token {

class Symbol: public Token {
    public:
        Symbol (const std::string &content):Token(content) {};
        static Symbol *generate (const std::string &content);
        bool isUnary ( void ) { return false; };
};

}

#endif