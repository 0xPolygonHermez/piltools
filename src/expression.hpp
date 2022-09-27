#ifndef __PIL__EXPRESSION__HPP__
#define __PIL__EXPRESSION__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>

namespace pil {
    class Expression;
}

#include "operation.hpp"
#include "fr_element.hpp"
#include "engine.hpp"
#include "expressions.hpp"

namespace pil {
    class Expression {
        public:

            uint64_t id = 0;
            char name[64] = "";
            int opIndex = 0;
            Operation operations[16];
            Expressions *parent;
            FrElement getEvaluation(uint64_t w) const;
            void precompile(nlohmann::json& node, int destination = 0, OperationType opType = OperationType::NONE );
            void dump(void);
            void eval(Engine &engine);
    };
}
#endif