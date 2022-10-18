#ifndef __TOKEN__HPP__
#define __TOKEN__HPP__

#include <string>

namespace pil {

class Token {
    public:
        const std::string content;
        Token (const std::string &content):content(content) {};
        virtual ~Token (void) {};
        static Token *generate (const std::string &content) { return new Token(content); };
        virtual int type ( void ) { return 0; };
        static int indexInList (const std::string &list, const std::string &value, char separator = ' ');
};

}
#endif