#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <list>
#include <typeinfo>
#include "tools.hpp"
#include "parser.hpp"
#include "token.hpp"

namespace pil {

Parser::TokenEntry Parser::tokenTable[14] = {
    {"==", EQ},
    {"=", NEQ},
    {"<=", LE},
    {">=", GE},
    {"!", '!'},
    {"=", '='},
    {"<", '>'},
    {">", '>'},
    {"+", '+'},
    {"-", '-'},
    {"*", '*'},
    {",", ','},
    {"(", '('},
    {")", ')'}};

const char *Parser::separators = "\n\t ";



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
    Code code;
    parseInit(code);
}


void Parser::clearTokens (void)
{
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        delete *it;
    }
    tokens.clear();
}

void Parser::parseInit (Code &result)
{
    std::cout << "parseInit " << std::endl;
    itoken = tokens.begin();
    if (getTokenType(0) == ID && getTokenType(1) == '(') {
        nextToken(2);
        parseFunction(result);
    } else {
        parseExpression(result);
    }
    parseExpression(result);
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
    return "\x1B[1;44;37m" +result+"\x1B[0m";
}

void Parser::parseExpression (Code &result)
{
    std::cout << "parseExpression " << remainingTokens() << std::endl;
    /* unitary_operator expr */
    /* ( expr ) */
    /* expr op expr */
    /* id */
    /* number */
    uint t0 = getTokenType(0);
    std::cout << "t0=" << t0 << std::endl;
    switch (t0) {

        case NUM:
            break;

        case ID:
            break;

        case '!':
            nextToken(1);
            parseExpression(result);
            return;

        case '(':
            nextToken(1);
            parseExpression(result);
            return;

        default:
            std::cout << "ERROR not found t0:" << t0 << std::endl;
            break;
    }

    uint t1 = getTokenType(1);
    std::cout << "t1=" << t1 << std::endl;
    switch (t1) {

        case '+':
        case '-':
        case '*':
        case EQ:
        case NEQ:
        case LE:
        case GE:
            nextToken(2);
            parseExpression(result);
            return;
        default:
            std::cout << "ERROR not found " << t1 << std::endl;
            break;
    }
}

void Parser::parseFunction (Code &result)
{
    std::cout << "parseFunction " << remainingTokens() << std::endl;
    uint argumentIndex = 0;
    while (getTokenType(0) != ')') {
        Code arg;
        if (argumentIndex) {
            if (getTokenType(0) != ',') {
                std::cout << "Expecting , but found " << getTokenContent(0) << std::endl;
                throw new ParserException("Expecting , but found "+ getTokenContent(0));
            }
            nextToken(1);
        }
        parseExpression(arg);
        std::cout << "> parseFunction " << remainingTokens() << std::endl;
        result.merge(arg);
        std::cout << "aftermerge" << std::endl;
        ++argumentIndex;
    }
}



void Parser::tokenize (const std::string &program)
{
    static const char *hexdigits = "0123456789ABCDEFabcdef";
    static const char *decdigits = "0123456789";

    const char *cprog = program.c_str();
    const char *end = cprog + program.size();

    uint tokenTableCount = sizeof(tokenTable) / sizeof(tokenTable[0]);

    while (cprog < end) {
        // separators
        while (cprog < end && strchr(separators, *cprog) != NULL) ++cprog;

        uint itoken;
        for (itoken = 0; itoken < tokenTableCount; ++itoken) {
            uint index = 0;

            while (tokenTable[itoken].pattern[index] && cprog[index] && tokenTable[itoken].pattern[index] == cprog[index]) {
                ++index;
            }
            if (!tokenTable[itoken].pattern[index] || tokenTable[itoken].pattern[index] == cprog[index]) {
                tokens.push_back(new Token(tokenTable[itoken].tokenid, tokenTable[itoken].pattern));
                cprog += strlen(tokenTable[itoken].pattern);
                break;
            }
        }
        if (itoken < tokenTableCount) continue;

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
            tokens.push_back(new Token(NUM, value));
            continue;
        }
        if (cprog < end && ((*cprog >= 'A' && *cprog <= 'Z') || (*cprog >= 'a' && *cprog <= 'z') || strchr("_%",*cprog))) {
            const char *from = cprog;
            ++cprog;
            while (cprog < end && ((*cprog >= 'A' && *cprog <= 'Z') || (*cprog >= 'a' && *cprog <= 'z') || (*cprog >= '0' && *cprog <= '9') || strchr("_[].%", *cprog))) {
                ++cprog;
            }
            std::string value(from, cprog-from);
            tokens.push_back(new Token(ID, value));
            continue;
        }
        if (cprog < end) {
            std::string value(cprog);
            throw std::runtime_error("token not found: "+value);
        }
    }

    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        std::cout << "TOKEN " << (*it)->type << " " << (*it)->content << std::endl;
    }
}

uint Parser::getTokenType (uint steps)
{
    auto it = itoken;
    while (steps > 0) {
        ++it;
        --steps;
    }

    return (*it)->type;
}

std::string Parser::getTokenContent (uint steps)
{
    auto it = itoken;
    std::cout << "\x1B[32mgetTokenContent(" << steps << "," << ")" << std::flush;
    while (steps > 0 && it != tokens.end()) {
        ++it;
        --steps;
    }

    std::string res = ((it != tokens.end()) ? (*it)->content : "(EOF)");
    std::cout << res << "\x1B[0m" << std::endl;
    return res;
}

void Parser::nextToken (uint steps)
{
    std::cout << "\x1B[33mnextToken(" << steps << "," << "," << std::flush;
    while (steps > 0 && itoken != tokens.end()) {
        ++itoken;
        --steps;
    }
    std::cout << steps << ")\x1B[0m" << std::endl;
}

}
