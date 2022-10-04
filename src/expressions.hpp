#ifndef __PIL__EXPRESSIONS__HPP__
#define __PIL__EXPRESSIONS__HPP__

namespace pil {
    class Expressions;
}

#include "reference.hpp"
#include "fr_element.hpp"
#include "dependencies.hpp"
#include "expression.hpp"
#include "operation_value.hpp"

namespace pil {

class Expressions {
    public:
        Dependencies dependencies;
        Expression *expressions;

        const Reference *add (const std::string &name, const Reference &value);
        const Reference *get (uint id) { return values + id; };
        void map (void *data) { evaluations = (FrElement *)data; }
        FrElement getEvaluation (uint id, uint w);
        const std::string &getName (uint id) { return values[id].name; }
        void calculateDependencies (void);
        void recursiveCalculateDependencies (uid_t expressionId);
        uint reduceNumberAliasExpressions (void);
        uint reduceAliasExpressions (void);
        uint replaceOperationValue (OperationValueType oldValueType, uint64_t oldValue, OperationValueType newValueType, uint64_t newValue);
        void loadAndCompile (nlohmann::json &pilExpressions);
        void evalAll (Engine &engine);
        void debugEval (Engine &engine, uid_t expressionId, omega_t w = 0);
        void dumpExpression (uid_t expressionId) { expressions[expressionId].dump(); };
        Expressions ( void );
        ~Expressions ( void );
    protected:
        uint count = 0;
        Reference *values = NULL;
        FrElement *evaluations;
        void compileExpression(nlohmann::json &pilExpressions, uid_t id);
};

}
#endif