#ifndef __PARSER__HPP__
#define __PARSER__HPP__

#include <string>
#include <stdexcept>
#include "token.hpp"
#include "code.hpp"

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

/*
    static const char *operators = "!=<>+*-(),";
    static const char *operators2B = "!= == <= >=";
    static const char *separators = "\n\t ";
    static const char *hexdigits = "0123456789ABCDEFabcdef";
    static const char *decdigits = "0123456789";
*/
class Parser {
    public:
        enum TokenId { EQ = 256, NEQ, GE, LE, NUM, ID };
        struct TokenEntry {
            const char *pattern;
            uint tokenid;
        };

        static TokenEntry tokenTable[14];
        static const char *separators;

        Parser (void);
        ~Parser (void);
        void compile (const std::string &program);
    protected:
        std::list<Token *> tokens;
        std::list<Token *>::iterator itoken;
        void tokenize (const std::string &program);
        void parseInit (Code &result);
        void parseExpression (Code &result);
        void parseFunction (Code &result);
        std::string remainingTokens (void);
        void clearTokens (void);
        uint getTokenType (uint steps = 0);
        std::string getTokenContent (uint steps = 0);
        void nextToken (uint steps = 1);
};


}


#endif