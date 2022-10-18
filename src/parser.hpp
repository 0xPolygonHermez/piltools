#ifndef __PARSER__HPP__
#define __PARSER__HPP__

#include <string>
#include <stdexcept>
#include "token.hpp"

namespace pil {

class ParserException : public std::exception {
    public:
        ParserException (const std::string &message): message(message) {};
        const char * what () {
            return message.c_str();
        }
    protected:
        std::string message;
};

class Parser {
    public:
        Parser (void);
        ~Parser (void);
        void compile (const std::string &program);
    protected:
        std::list<Token *> tokens;
        std::list<Token *>::iterator itoken;
        void tokenize (const std::string &program);
        void parseInit (void);
        void parseExpression (void);
        void parseFunction (void);
        std::string remainingTokens (void);
        void clearTokens (void);
};


}


#endif