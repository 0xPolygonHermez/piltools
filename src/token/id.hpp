#ifndef __TOKEN__ID__HPP__
#define __TOKEN__ID__HPP__

#include <string>
#include "../token.hpp"

namespace pil::token {

class Id: public Token {
    public:
        Id (const std::string &content):Token(content) {};
        static Token *generate (const std::string &content) { return new Id(content); };
};

}

#endif