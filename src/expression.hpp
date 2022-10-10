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
            uid_t groupId;
            std::string name;
            dim_t next;
            bool nextExpression;

            std::vector<Operation> operations;
            Dependencies dependencies;
            // Expressions *parent;
            bool alias;
            bool compiled;
            bool isZero;
            bool evaluated;
            omega_t firstNonZeroEvaluation;
            FrElement *aliasEvaluations;

            Expression (void);
            bool compile (nlohmann::json &node);
            void dump (void) const;
            FrElement eval (Engine &engine, omega_t w = 0, bool debug = false);
            FrElement getAliasEvaluation (Engine &engine, omega_t w, uid_t evalGroupId, bool debug = false);
            OperationValueType getAliasType ( void ) const;
            FrElement getAliasValue ( void ) const;
            uint64_t getAliasValueU64 ( void ) const;
            bool isAlias ( void ) const { return alias; };
            uint replaceOperationValue(OperationValueType oldValueType, uint64_t oldValue, OperationValueType newValueType, uint64_t newValue);
            void setIsZeroFlag ( bool value, omega_t w = 0 ) { isZero = value; firstNonZeroEvaluation = w; };
            omega_t getFirstNonZeroEvaluation ( void ) { return firstNonZeroEvaluation; };
            void setEvaluatedFlag ( bool value ) { evaluated = value; };
        protected:
            void recursiveCompile (nlohmann::json& node, dim_t destination, OperationType opType);
            dim_t getFreeId (void);
    };
}

#endif