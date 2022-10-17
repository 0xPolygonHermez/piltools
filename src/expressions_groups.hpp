#ifndef __PIL__EXPRESSIONS_GROUPS_HPP__
#define __PIL__EXPRESSIONS_GROUPS_HPP__

namespace pil {
    class ExpressionsGroups;
}

#include "reference.hpp"
#include "fr_element.hpp"
#include "dependencies.hpp"
#include "expression.hpp"
#include "expressions.hpp"
#include "operation_value.hpp"
#include "omp.h"

namespace pil {

class ExpressionsGroups {
    public:
        static const uid_t GROUP_NONE = 0x7FFFFFFF;

        void calculate (void);
        ExpressionsGroups (Expressions &expressions, Dependencies &dependencies);
        ~ExpressionsGroups (void);
        bool isZero(uid_t id);
    protected:
        FrElement *evaluations;
        Expressions &expressions;
        Dependencies &dependencies;
        uint64_t evaluationsDone;
        uint activeCpus;
        class EvalGroup {
            public:
                uid_t groupId;
                omega_t w1;
                omega_t w2;
                uint64_t cost;
                uint icpu;
                std::list<uid_t> expressions;
        };
        class ExpressionGroup {
            public:
                std::list<uid_t> expressions;
        };
        std::list<EvalGroup> evalGroups;
        std::vector<ExpressionGroup> groups;

        void resetGroupId (void);
        void mergeGroups (uid_t toId, uid_t fromId);
        void recursiveSetGroup (uid_t exprId, uid_t groupId);
        void updateWithDependencies ( void );
        void compactGroupId (void);
        void calculateGroupId (void);
        uid_t *initGroupIdTranslationTable (void);
        uid_t getTranslatedGroupId (uid_t *translateIds, uid_t oldGroupId);
        void distributeEvaluationGroupsAmoungCpus (void);
        void generateEvaluationGroups (void);
};

}
#endif