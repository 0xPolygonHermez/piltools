#ifndef __PIL__EXPRESSION__HPP__
#define __PIL__EXPRESSION__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>
#include <vector>
namespace pil {
    class Expression;
}

#include "operation.hpp"
#include "fr_element.hpp"
#include "engine.hpp"
// #include "expressions.hpp"
#include "dependencies.hpp"

namespace pil {
    class Expression {
        public:

            uid_t id;
            std::string name;
            dim_t next;

            std::vector<Operation> operations;
            // Expressions *parent;
            bool alias;
            bool compiled;
            FrElement *aliasEvaluations;

            FrElement getEvaluation(omega_t w) const;

            void compile (nlohmann::json& node, Dependencies &dependencies);
            void dump (void);
            void eval (Engine &engine);
            Expression (void);
        protected:
            void recursiveCompile (nlohmann::json& node, dim_t destination, OperationType opType, Dependencies &dependencies);
            dim_t getFreeId (void);
    };
}

#endif