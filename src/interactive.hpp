#ifndef __INTERACTIVE__HPP__
#define __INTERACTIVE__HPP__

#include <string>
#include <vector>
#include "engine.hpp"
#include "types.hpp"

namespace pil {

class Interactive {
    public:
        Interactive (Engine &engine);
        void execute (void);
    protected:
        omega_t w;
        std::string cmd;
        std::vector<std::string> argv;
        Engine &engine;
        std::string prompt (void);
        void parseInput (const std::string &input);

        void cmdSet (void);
        void cmdPrint (void);
        void cmdList (void);
        void cmdHelp (void);
        void cmdFind (void);
};

}
#endif