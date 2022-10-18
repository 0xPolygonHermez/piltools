#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <string.h>
#include <list>
#include <typeinfo>
#include "tools.hpp"
#include "parser.hpp"
#include "token/numeric.hpp"
#include "token/id.hpp"
#include "token/symbol.hpp"

namespace pil {


Parser::Parser (void)
{
}

Parser::~Parser (void)
{

}

void Parser::compile (const std::string &program)
{
    clearTokens();
    tokenize(program);
    parseInit();
}


void Parser::clearTokens (void)
{
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        delete *it;
    }
    tokens.clear();
}

void Parser::parseInit (void)
{
    std::cout << "parseInit " << std::endl;
    itoken = tokens.begin();
    std::cout << typeid(**itoken).name() << "\"" << (*itoken)->content << "\"" << instanceof<token::Id>(*itoken) << std::endl;
    if (instanceof<token::Id>(*itoken)) {
        auto next = itoken;
        ++next;
        std::cout << "NEXT" << typeid(**next).name() << "\"" << (*next)->content << "\"" << std::endl;
        if (next != tokens.end() && (*next)->content == "(") {
            ++itoken;
            ++itoken;
            parseFunction();
        }
    }
    parseExpression();
}

std::string Parser::remainingTokens ( void )
{
    auto it = itoken;
    std::string result;

    while (it != tokens.end()) {
        result += (*it)->content;
        result += " ";
        ++it;
    }
    return "\x1B[46m" +result+"\x1B[0m";
}

void Parser::parseExpression ( void )
{
    std::cout << "parseExpression " << remainingTokens() << std::endl;
    auto &token = **itoken;
    /* unitary_operator expr */
    /* ( expr ) */
    /* expr op expr */
    /* id */
    /* number */

    if (instanceof<token::Numeric>(*itoken)) {
        ++itoken;
        return;
    }

    if (instanceof<token::Id>(*itoken)) {
        ++itoken;
        return;
    }

    if (instanceof<token::Symbol>(*itoken) && ((token::Symbol *)(*itoken))->isUnary()) {
        ++itoken;
        parseExpression();
        return;
    }

    if (token.content == "(") {
        ++itoken;
        parseExpression();
        return;
    }
}

void Parser::parseFunction ( void )
{
    std::cout << "parseFunction " << remainingTokens() << std::endl;
    bool argumentIndex = 0;
    while (itoken != tokens.end() && (*itoken)->content != ")") {
        auto &token = **itoken;
        if (argumentIndex) {
            if (token.content != ",") {
                throw ParserException("Expecting , but found "+ token.content);
            }
            ++itoken;
        }
        parseExpression();
        std::cout << "> parseFunction " << remainingTokens() << std::endl;
    }
}

void Parser::tokenize (const std::string &program)
{
    static const char *operators = "!=<>+*-(),";
    static const char *operators2B = "!= == <= >=";
    static const char *separators = "\n\t ";
    static const char *hexdigits = "0123456789ABCDEFabcdef";
    static const char *decdigits = "0123456789";

    const char *cprog = program.c_str();
    const char *end = cprog + program.size();
    char tmp2B[4] = "XX ";

    while (cprog < end) {
        while (cprog < end && strchr(separators, *cprog) != NULL) ++cprog;
        if (cprog < end && strchr(operators, *cprog)) {
            if ((cprog + 1) < end) {
                tmp2B[0] = *cprog;
                tmp2B[1] = *(cprog+1);

                if (strstr(operators2B, tmp2B) != NULL) {
                    std::string value(tmp2B, 2);
                    tokens.push_back(token::Symbol::generate(value));
                    cprog += 2;
                    continue;
                }
            }
            std::string value(cprog++, 1);
            tokens.push_back(token::Symbol::generate(value));
            continue;
        }
        if (cprog < end && *cprog >= '0' && *cprog <= '9') {
            const char *from = cprog;
            if ((cprog + 2) < end && *cprog == '0' && (cprog[1] == 'x' || cprog[1] == 'X')) {
                cprog += 2;
                while (cprog < end && strchr(hexdigits, *cprog) != NULL) ++cprog;
            }
            else {
                while (cprog < end && strchr(decdigits, *cprog) != NULL) ++cprog;
            }
            std::string value(from, cprog-from);
            tokens.push_back(token::Numeric::generate(value));
            continue;
        }
        if (cprog < end && ((*cprog >= 'A' && *cprog <= 'Z') || (*cprog >= 'a' && *cprog <= 'z') || strchr("_%",*cprog))) {
            const char *from = cprog;
            ++cprog;
            while (cprog < end && ((*cprog >= 'A' && *cprog <= 'Z') || (*cprog >= 'a' && *cprog <= 'z') || (*cprog >= '0' && *cprog <= '9') || strchr("_[].%", *cprog))) {
                ++cprog;
            }
            std::string value(from, cprog-from);
            tokens.push_back(token::Id::generate(value));
            continue;
        }
        if (cprog < end) {
            std::string value(cprog);
            throw std::runtime_error("token not found: "+value);
        }
    }

    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        std::cout << typeid(**it).name() << "[" << (*it)->content << "]" << std::endl;
    }
}
}
