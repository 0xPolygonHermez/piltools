#ifndef __CODE__HPP__
#define __CODE__HPP__

#include <string>
#include <list>
namespace pil {

class Code {
    public:
        std::list<std::string> instr;
        void merge (Code &c2) { instr.splice(instr.end(), c2.instr); };
};

}
#endif