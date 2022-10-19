#include "interactive.hpp"

#include <string>
#include <iostream>

#include <editline/readline.h>
#include <editline/history.h>

namespace pil {

Interactive::Interactive (Engine &engine)
    :engine(engine)
{
    w = 0;
}

std::string Interactive::prompt ( void )
{
    std::stringstream _prompt;

    _prompt << "\001\x1B[1;36m\002PIL[w:" << w << "]>\001\x1B[0m\002 ";
    return _prompt.str();
}

void Interactive::execute ( void )
{
    bool quit = false;
    std::string input;

    while (!quit) {

        // std::getline(std::cin, input);
        input = readline(prompt().c_str());
        parseInput(input);
        if (cmd == "q" || cmd == "quit") {
            break;
        }
        if (!input.empty()) {
            add_history(input.c_str());
        }
        if (cmd == "p" || cmd == "print") cmdPrint();
        else if (cmd == "s" || cmd == "set") cmdSet();
        else if (cmd == "f" || cmd == "find") cmdFind();
        else if (cmd == "l" || cmd == "list") cmdList();
        else if (cmd == "h" || cmd == "help") cmdHelp();
        else {
            std::cout << "ERROR: unknown command " << argv[1] << std::endl;
        }
    }
}

void Interactive::cmdSet (void)
{
    if (argv[1] == "w") {
        w = strtoull(argv[2].c_str(), NULL, 0);
    }
}

void Interactive::cmdPrint (void)
{
    for (uint index = 1; index < argv.size(); ++index) {
        if (strcasecmp(argv[index].c_str(), "hex") == 0) {
            std::cout << std::hex;
            continue;
        }
        else if (strcasecmp(argv[index].c_str(), "dec") == 0) {
            std::cout << std::dec;
            continue;
        }
        try {
            FrElement value = engine.getEvaluation(argv[index], w);
            std::cout << argv[index] << " = " << Goldilocks::toString(value) << std::endl;
        } catch (const std::exception& e) {
            std::cout << "\x1B[1;31m ERROR: " << e.what() << "\x1B[0m" << std::endl;
        }
    }
    std::cout << std::dec;
}

void Interactive::cmdFind (void)
{
    // TODO = decode (name, index, w);
    auto ref = engine.getReference(argv[1]);
    FrElement value = Goldilocks::fromString(argv[2]);
    auto w1 = w;
    while (w1 < engine.n && !Goldilocks::equal(ref->getEvaluation(w1), value)) ++w1;
    if (w1 < engine.n) {
        std::cout << "Value found on w=" << w1 << std::endl;
    } else {
        std::cout << "\x1B[1;31m ERROR: Not found " << argv[1] << " == " << Goldilocks::toString(value) << " from current omega (w=" << w << ")\x1B[0m" << std::endl << std::endl;
    }
}

void Interactive::cmdList (void)
{

}

void Interactive::cmdHelp (void)
{

}

void Interactive::parseInput (const std::string &input)
{
    std::istringstream is(input);
    argv.clear();
    std::string field;
    while (!is.eof()) {
        getline( is, field, ' ');
        if (field.empty()) continue;
        argv.push_back(field);
    }
    cmd = argv[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
        [](unsigned char c){ return std::tolower(c); });
}

}