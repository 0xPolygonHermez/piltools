#include <sys/time.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <list>
#include <algorithm>
#include <sstream>
#include <stdint.h>
#include "omp.h"

#include "expressions_groups.hpp"
#include "expressions.hpp"
#include "operation_value.hpp"
#include "operation.hpp"
#include "engine.hpp"
#include "fr_element.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "block.hpp"

namespace pil {

ExpressionsGroups::ExpressionsGroups (Expressions &expressions, Dependencies &dependencies)
    :expressions(expressions), dependencies(dependencies)
{
}

ExpressionsGroups::~ExpressionsGroups (void)
{
}

void ExpressionsGroups::calculate (void)
{
    resetGroupId();
    calculateGroupId();
    compactGroupId();
    dim_t count = expressions.size();
    uid_t *groupTranslate = new uid_t[count];


    dim_t *groupCounters = new dim_t[count]();
    uint64_t *groupCosts = new uint64_t[count]();
    uint64_t totalCost = 0;
    bool *groupHasNextExpressions = new bool[count]();

//    for (index_t index = 0; index < count; ++index) {
    for (uid_t idep = 0; idep < dependencies.size(); ++idep){
        const uid_t iexpr = dependencies[idep];

        auto _groupId = expressions[iexpr].groupId;
        ++groupCounters[_groupId];
        const dim_t cost = expressions[iexpr].operations.size();
        groupCosts[_groupId] += cost;
        totalCost += cost;
        if (expressions[iexpr].nextExpression) {
            groupHasNextExpressions[_groupId] = true;
        }
    }

    dim_t groupsWithElements = 0;
    std::list<uid_t> groupsByCost;
    for (index_t index = 0; index < count; ++index) {
        const dim_t expressionsInGroup = groupCounters[index];
        if (!expressionsInGroup) continue;
        ++groupsWithElements;
        groupsByCost.push_back(index);
    }
    groupsByCost.sort( [groupCosts]( const uid_t &a, const uid_t &b ) { return groupCosts[a] > groupCosts[b]; } );

    dim_t *cpuGroupCosts = new dim_t[cpus]();
    dim_t cpuGroupCostTarget = (totalCost / cpus) + 1;

    std::cout << "cpuGroupCostTarget: " << cpuGroupCostTarget << std::endl;

    for (auto it = groupsByCost.begin(); it != groupsByCost.end(); ++it) {
        EvalGroup eg = {groupId: *it, w1: 0, w2: n, cost: groupCosts[*it]};
        uint parts = cpuGroupCostTarget / groupCosts[*it];
        if (parts <= 1) {
            evalGroups.push_back(eg);
            continue;
        }
        omega_t wpart = n / parts;
        eg.cost = eg.cost / parts;
        for (uint ipart = 0; ipart < parts; ++ipart) {
            eg.w1 = wpart * ipart;
            eg.w2 = (ipart == (parts - 1)) ? n : (eg.w1 + wpart);
            evalGroups.push_back(eg);
        }
    }

    updateEvalGroupWithDependencies();


    // TODO: if no next expressions => split in n parts the big groups

    std::cout << "Calculated dependencies groups (cpus:" << cpus << ")" << std::endl;
    std::cout << "Groups(we): " << groupsWithElements << "  cpuGroupCostTarget: " << cpuGroupCostTarget << "  totalCost: " << totalCost << std::endl;

    delete [] groupCounters;
    delete [] cpuGroupCosts;
}

void ExpressionsGroups::distributeEvaluationGroupsAmoungCpus (void)
{
    auto it = evalGroups.begin();
    uint loop = 0;
    bool overbooking = false;
    for (uint loop = 0; it != evalGroups.end(); ++loop) {
        uint assignments = 0;
        for (uint icpu = 0; (icpu < cpus) && it != evalGroups.end(); ++icpu) {
            if (!overbooking && cpuGroupCosts[icpu] >= cpuGroupCostTarget) continue;
            ++assignments;
            cpuGroupCosts[icpu] += it->cost;
            it->icpu = icpu;
            ++it;
        }
        overbooking = (assigments == 0);
    }
}


void ExpressionsGroups::resetGroupId ( void )
{
    // TODO: free memory + recallable
    for (uid_t id = 0; id < expressions.size(); ++id) {
        expressions[id].groupId = GROUP_NONE;
    }
}


void ExpressionsGroups::mergeGroups (uid_t toId, uid_t fromId)
{
    for (uid_t id = 0; id < expressions.size(); ++id) {
        if (expressions[id].groupId == fromId) expressions[id].groupId = toId;
    }
}


void ExpressionsGroups::recursiveSetGroup (uid_t exprId, uid_t groupId)
{
    if (expressions[exprId].groupId != GROUP_NONE && expressions[exprId].groupId != groupId) {
        mergeGroups(groupId, expressions[exprId].groupId);
        return;
    }

    for (uid_t idep = 0;  idep < expressions[exprId].dependencies.size(); ++idep) {
        recursiveSetGroup(expressions[exprId].dependencies[idep], groupId);
    }
    expressions[exprId].groupId = groupId;
}


void ExpressionsGroups::calculateGroupId (void)
{
    uid_t groupId = 0;
    for (uid_t id = 0; id < expressions.size(); ++id) {
        if (expressions[id].groupId != GROUP_NONE) continue;
        recursiveSetGroup(id, groupId);
        ++groupId;
    }
}

uid_t *ExpressionsGroups::initGroupIdTranslationTable (void)
{
    uid_t *translateIds = new uid_t[expressions.size()];
    for (uid_t id = 0; id < expressions.size(); ++id) {
        translateIds[id] = GROUP_NONE;
    }
    return translateIds;
}

uid_t ExpressionsGroups::getTranslatedGroupId (uid_t *translateIds, uid_t oldGroupId)
{
    if (translateIds[oldGroupId] == GROUP_NONE) {
        const auto newGroupId = groups.size();
        translateIds[oldGroupId] = newGroupId;
        groups.emplace_back();
        return newGroupId;
    }
    return translateIds[oldGroupId];
}

void ExpressionsGroups::compactGroupId (void)
{
    dim_t count = expressions.size();
    uid_t *translateIds = initGroupIdTranslationTable();
    groups.clear();

    uid_t newGroupId;
    for (uid_t id = 0; id < dependencies.size(); ++id) {
        const auto exprId = dependencies[id];
        const auto oldGroupId = expressions[exprId].groupId;

        assert(oldGroupId != GROUP_NONE);

        const auto newGroupId = getTranslatedGroupId(translateIds, oldGroupId);

        expressions[exprId].groupId = newGroupId;
        groups[newGroupId].expressions.push_back(exprId);
    }
    delete [] translateIds;
}



void ExpressionsGroups::updateEvalGroupWithDependencies ( void )
{
    for (auto it = evalGroups.begin())
}


}
