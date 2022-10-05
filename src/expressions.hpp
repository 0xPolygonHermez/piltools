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
#include "omp.h"

namespace pil {

class Expressions {
    public:
        static const uid_t GROUP_NONE = 0x7FFFFFFF;

        Dependencies dependencies;
        Expression *expressions;
        dim_t cpus;
        std::list<uid_t> *cpuGroups;
        dim_t n;
        uint count = 0;
        Engine &engine;

        const Reference *add (const std::string &name, const Reference &value);
        const Reference *get (uint id) { return values + id; };
        void map (void *data) { evaluations = (FrElement *)data; }
        FrElement getEvaluation (uid_t id, omega_t w, uid_t evalGroupId);
        const std::string &getName (uint id) { return values[id].name; }
        void calculateDependencies (void);
        void recursiveCalculateDependencies (uid_t expressionId);
        uint reduceNumberAliasExpressions (void);
        uint reduceAliasExpressions (void);
        uint replaceOperationValue (OperationValueType oldValueType, uint64_t oldValue, OperationValueType newValueType, uint64_t newValue);
        void loadAndCompile (nlohmann::json &pilExpressions);
        void evalAll (void);
        void evalAllCpuGroup(uid_t icpu);
        void debugEval (uid_t expressionId, omega_t w = 0);
        void dumpExpression (uid_t expressionId) { expressions[expressionId].dump(); };
        void dumpDependencies ( void );
        void calculateGroup (void);
        Expressions (Engine &engine);
        ~Expressions ( void );
        bool isZero(uid_t id);
    protected:
        Reference *values = NULL;
        FrElement *evaluations;
        void compileExpression (nlohmann::json &pilExpressions, uid_t id);
        void resetGroups (void);
        void mergeGroup (uid_t toId, uid_t fromId);
        void recursiveSetGroup (uid_t exprId, uid_t groupId);
};

}
#endif