#include <sys/time.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include <algorithm>
#include "omp.h"

#include "expressions.hpp"
#include "operation_value.hpp"
#include "operation.hpp"
#include "engine.hpp"
#include "types.hpp"

namespace pil {

uint Expressions::reduceAliasExpressions (void)
{
    uint replacements = 0;
    for (uint aliasId = 0; aliasId < count; ++aliasId) {
        OperationValueType vType = expressions[aliasId].getAliasType();
        if (vType == OperationValueType::NONE) continue;
        uid_t finalAliasId = aliasId;
        while ((vType = expressions[finalAliasId].getAliasType()) == OperationValueType::EXP) {
            finalAliasId = expressions[finalAliasId].getAliasValueU64();
        }
        switch(vType) {
            case OperationValueType::CM:
            case OperationValueType::CONST:
            case OperationValueType::PUBLIC:
            case OperationValueType::NUMBER:
                replacements += replaceOperationValue(OperationValueType::EXP, aliasId, vType, expressions[finalAliasId].getAliasValueU64());
                break;
            case OperationValueType::NONE:
                // it isn't an alias
                replacements += replaceOperationValue(OperationValueType::EXP, aliasId, OperationValueType::EXP, finalAliasId);
                break;
            default:
                throw std::runtime_error("Invalid alias chain Type");
        }
    }
    std::cout << replacements << " replacements on reduceAliasExpressions" << std::endl;
    return replacements;
}

uint Expressions::replaceOperationValue(OperationValueType oldValueType, uint64_t oldValue, OperationValueType newValueType, uint64_t newValue)
{
    uint replacements = 0;
    for (uint index = 0; index < count; ++index) {
        replacements += expressions[index].replaceOperationValue(oldValueType, oldValue, newValueType, newValue);
    }
    return replacements;
}

uint Expressions::reduceNumberAliasExpressions (void)
{
    uint count = 0;
    for (uint aliasId = 0; aliasId < count; ++aliasId) {
        if (expressions[aliasId].getAliasType() != OperationValueType::NUMBER) continue;
        count += replaceOperationValue(OperationValueType::EXP, aliasId, OperationValueType::NUMBER, expressions[aliasId].getAliasValueU64());
    }
    std::cout << count << " replacements" << std::endl;
    return count;
}

void Expressions::calculateDependencies (void)
{
    for (uint index = 0; index < count; ++index) {
        recursiveCalculateDependencies(index);
    }
    std::cout << dependencies.size() << " " << count << std::endl;
}

void Expressions::recursiveCalculateDependencies (uid_t expressionId)
{
    if (dependencies.contains(expressionId)) return;
    Expression &expression = expressions[expressionId];

    for (uint index = 0; index < expression.dependencies.size(); ++index) {
        recursiveCalculateDependencies(expression.dependencies[index]);
    }
    dependencies.add(expressionId);
}

FrElement Expressions::getEvaluation(uid_t id, omega_t w, uid_t evalGroupId)
{
    if (id >= count) {
        throw std::runtime_error("Out of range on expressions accessing id "+std::to_string(id)+" but last id was "+std::to_string(count - 1));
    }

    if (expressions[id].isAlias()) {
        return expressions[id].getAliasEvaluation(engine, w, evalGroupId);
    }

    if (evalGroupId != GROUP_NONE && expressions[id].groupId != evalGroupId) {
        std::cout << "evalGroupId:" << evalGroupId << " expressions[" << id << "].groupId:" << expressions[id].groupId << std::endl;
        assert(false);
        exit(0);
    }
    if (!expressions[id].evaluated) {
        std::cout << "no evaluated expression id:" << id << " evalGroupId:" << evalGroupId << std::endl;
        assert(false);
        exit(0);
    }
/*  TO VERIFY
    if (expressions[id].isAlias()) {
        return expressions[id].getEvaluation(w);
    }*/
    return evaluations[ (uint64_t)id * n + w];
}

bool Expressions::isZero(uid_t id)
{
    if (id >= count) {
        throw std::runtime_error("Out of range on expressions accessing id "+std::to_string(id)+" but last id was "+std::to_string(count - 1));
    }

    if (expressions[id].isAlias()) {
        throw std::runtime_error("Alias not supported");
    }
    return expressions[id].isZero;
}

void Expressions::loadAndCompile(nlohmann::json &pilExpressions)
{
    count = pilExpressions.size();
    expressions = new Expression[count];

    for (uid_t index = 0; index < count; ++index) {
        expressions[index].id = index;
    }

    for (uid_t index = 0; index < count; ++index) {
        std::cout << "========== compiling " << index << "/" << count <<  " ==========" << std::endl;
        compileExpression(pilExpressions, index);
        // e.eval(*this);
    }
    dependencies.dump();
}

void Expressions::dumpDependencies (void)
{
    for (uid_t id = 0; id < count; ++id) {
        std::cout << "ED[" << id << "] => ";
        expressions[id].dependencies.dump();
    }
}

void Expressions::compileExpression (nlohmann::json &pilExpressions, uid_t id)
{
    if (!expressions[id].compile(pilExpressions[id])) return;
    if (id == 0) expressions[id].dump();

    const uint dependenciesCount = expressions[id].dependencies.size();
    for (uint idep = 0; idep < dependenciesCount; ++idep) {
        compileExpression(pilExpressions, expressions[id].dependencies[idep]);
    }
    #ifdef __DEBUG__
    std::cout << "dependecies[" << id << "]: " << dependenciesCount << " => ";
    for (uint idep = 0; idep < dependenciesCount; ++idep) {
        std::cout << expressions[id].dependencies[idep] << " ";
    }
    std::cout  << std::endl;
    #endif

    dependencies.merge(expressions[id].dependencies);
    dependencies.add(id);
}


void Expressions::debugEval(uid_t expressionId, omega_t w)
{
    expressions[expressionId].eval(engine, w, true);
}

void Expressions::evalAll(void)
{
    if (evaluations == NULL) {
        std::cout << "creating evaluations " << count * engine.n << "(" << count << "*" << engine.n << ")" << std::endl;
        evaluations = new FrElement[count * (uint64_t)n];
    }
    struct timeval time_now;
    gettimeofday(&time_now, nullptr);
    time_t startT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

    // std::cout << "omp_get_max_threads: " << omp_get_max_threads() << std::endl;
    #pragma omp parallel for
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        evalAllCpuGroup(icpu);
    }

    gettimeofday(&time_now, nullptr);
    time_t endT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
    std::cout << "time(ms):" <<  (endT - startT) << std::endl;
}

void Expressions::evalAllCpuGroup(uid_t icpu)
{
    dim_t depCount = dependencies.size();
    for (int idep = 0; idep < depCount; ++idep) {
        const uid_t iexpr = dependencies[idep];
        if (std::find(cpuGroups[icpu].begin(), cpuGroups[icpu].end(), expressions[iexpr].groupId) == cpuGroups[icpu].end()) continue;

        #pragma omp critical
        {
            std::cout << "[" << icpu <<"] CPU (E:" << iexpr << ") " << idep << "/" << depCount << std::endl;
        }

        for (omega_t w = 0; w < engine.n; ++w) {
            evaluations[ (uint64_t)n * iexpr + w ] = expressions[iexpr].eval(engine, w);
        }
        expressions[iexpr].evaluated = true;
        #ifdef __DEBUG__
        std::cout << "expression[" << iexpr << "] evaluated" << std::endl;
        #endif
    }
}

Expressions::Expressions (Engine &engine)
    :engine(engine)
{
    evaluations = NULL;
    expressions = NULL;
    cpus = 64;
    cpuGroups = new std::list<uid_t>[cpus];
}

Expressions::~Expressions (void)
{
    if (evaluations) delete [] evaluations;
    if (expressions) delete [] expressions;
}

void Expressions::resetGroups ( void )
{
    // TODO: free memory + recallable
    for (uid_t id = 0; id < count; ++id) {
        expressions[id].groupId = GROUP_NONE;
    }
}


void Expressions::mergeGroup (uid_t toId, uid_t fromId)
{
    for (uid_t id = 0; id < count; ++id) {
        if (expressions[id].groupId == fromId) expressions[id].groupId = toId;
    }
}


void Expressions::recursiveSetGroup (uid_t exprId, uid_t groupId)
{
    std::cout << "recursiveSetGroup(exprId:" << exprId << ", groupId:" << groupId << ") #" << expressions[exprId].dependencies.size() << std::endl;
    if (expressions[exprId].groupId != GROUP_NONE && expressions[exprId].groupId != groupId) {
        mergeGroup(groupId, expressions[exprId].groupId);
        return;
    }

    for (uid_t idep = 0;  idep < expressions[exprId].dependencies.size(); ++idep) {
        recursiveSetGroup(expressions[exprId].dependencies[idep], groupId);
    }
    expressions[exprId].groupId = groupId;
}

void Expressions::calculateGroup (void)
{
    resetGroups();
    uid_t groupId = 0;
    for (uid_t id = 0; id < count; ++id) {
        if (expressions[id].groupId != GROUP_NONE) continue;
        recursiveSetGroup(id, groupId);
        ++groupId;
    }

/*    const dim_t depCount = dependencies.size();
    for (int64_t idep = depCount - 1; idep >= 0; --idep) {
        const dim_t id = dependencies[idep];
        const uid_t groupId = expressions[id].groupId;
        const dim_t depCount2 = expressions[id].dependencies.size();
        for (dim_t idep2 = 0; idep2 < depCount2; ++idep2) {
            expressions[expressions[id].dependencies[idep2]].groupId = groupId;
        }
    }
    for (int64_t idep = 0; idep < depCount; ++idep) {
        const dim_t id = dependencies[idep];
        const uid_t groupId = expressions[id].groupId;
        const dim_t depCount2 = expressions[id].dependencies.size();
        for (dim_t idep2 = 0; idep2 < depCount2; ++idep2) {
            expressions[expressions[id].dependencies[idep2]].groupId = groupId;
        }
    }*/

    dim_t *groupCounters = new dim_t[count]();
    bool *groupHasNextExpressions = new bool[count]();

    for (index_t index = 0; index < count; ++index) {
        auto _groupId = expressions[index].groupId;
        ++groupCounters[_groupId];
        if (expressions[index].nextExpression) {
            groupHasNextExpressions[_groupId] = true;
        }
    }

    dim_t groupsWithElements = 0;
    std::list<uid_t> groupsBySizeDesc;
    for (index_t index = 0; index < count; ++index) {
        const dim_t expressionsInGroup = groupCounters[index];
        if (!expressionsInGroup) continue;
        ++groupsWithElements;
        groupsBySizeDesc.push_back(index);
    }
    groupsBySizeDesc.sort( [groupCounters]( const uid_t &a, const uid_t &b ) { return groupCounters[a] > groupCounters[b]; } );

    std::cout << "groups:" << groupsWithElements << std::endl;
    uint total = 0;
    for (auto it = groupsBySizeDesc.begin(); it != groupsBySizeDesc.end(); ++it) {
        std::cout << "G " << *it << " " << groupCounters[*it] << (groupHasNextExpressions[*it] ? " next:TRUE":" next:false") << std::endl;
        total += groupCounters[*it];
    }
    std::cout << "count: " << count << " total:" << total << std::endl;
    dim_t *cpuGroupSizes = new dim_t[cpus]();

    dim_t cpuGroupSizeTarget = (count / cpus) + ((count % cpus) != 0);
    std::cout << "cpuGroupSizeTarget:" << cpuGroupSizeTarget << std::endl;
    auto it = groupsBySizeDesc.begin();

    // TODO: if no next expressions => split in n parts the big groups
    dim_t groupSize;
    for (uint loop = 0; it != groupsBySizeDesc.end(); ++loop) {
        for (uint icpu = 0; (icpu < cpus) && it != groupsBySizeDesc.end(); ++icpu) {
            if (cpuGroupSizes[icpu] >= cpuGroupSizeTarget) continue;
            groupId = *it;
            groupSize = groupCounters[groupId];
            std::cout << "> adding group " << groupId << "(" << groupSize << ") to cpu " << icpu << "(" << cpuGroupSizes[icpu];
            cpuGroupSizes[icpu] += groupSize;
            std::cout << ") => " << cpuGroupSizes[icpu] << std::endl;
            cpuGroups[icpu].push_back(groupId);
            ++it;
        }
    }
    uint totalGroups = 0;
    total = 0;
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        totalGroups += cpuGroups[icpu].size();
        total += cpuGroupSizes[icpu];
        std::cout << "CPU " << icpu << " G:" << cpuGroups[icpu].size() << " E:" << cpuGroupSizes[icpu] << std::endl;
    }
    std::cout << "totalGroups:" << totalGroups << std::endl;

    delete [] groupCounters;
    delete [] cpuGroupSizes;
}


}