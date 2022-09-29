#ifndef __PIL__EXPRESSION__HPP__
#define __PIL__EXPRESSION__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>
#include <vector>

#include "operation.hpp"
#include "fr_element.hpp"
#include "dependencies.hpp"

namespace pil {
    class Expression {
        public:

            uid_t id;
            std::string name;
            dim_t next;

            std::vector<Operation> operations;
            Dependencies dependencies;
            // Expressions *parent;
            bool alias;
            bool compiled;
            FrElement *aliasEvaluations;

            FrElement getEvaluation(omega_t w) const;

            void compile (nlohmann::json& node);
            void dump (void);
            FrElement eval (Engine &engine, omega_t w = 0);
            Expression (void);
            OperationValueType getAliasType ( void );
            FrElement getAliasValue ( void );
            uint64_t getAliasValueU64 ( void );
            bool isAlias ( void ) { return alias; };
            uint replaceOperationValue(OperationValueType oldValueType, uint64_t oldValue, OperationValueType newValueType, uint64_t newValue);
        protected:
            void recursiveCompile (nlohmann::json& node, dim_t destination, OperationType opType);
            dim_t getFreeId (void);
    };
}

#endif