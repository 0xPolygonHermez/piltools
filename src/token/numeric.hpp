#ifndef __TOKEN__NUMERIC__HPP__
#define __TOKEN__NUMERIC__HPP__

#include <string>
#include "../token.hpp"

namespace pil::token {

class Numeric: public Token {
    public:
        Numeric (const std::string &content):Token(content) {};
        static Token *generate (const std::string &content) { return new Numeric(content); };
};

}

#endif