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

#include "expressions.hpp"
#include "operation_value.hpp"
#include "operation.hpp"
#include "engine.hpp"
#include "fr_element.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "block.hpp"

namespace pil {

std::string Expressions::getName (uid_t expressionId)
{
    uid_t id = expressionId;
    OperationValueType vtype;
    while ((vtype = expressions[id].getAliasType()) != OperationValueType::NONE) {
        id = expressions[id].getAliasValueU64();
        switch (vtype) {
            case OperationValueType::CONST:  return engine.getConstName(id);
            case OperationValueType::CM:     return engine.getCommitedName(id);
            case OperationValueType::PUBLIC:  return engine.getPublicName(id);
            default:
                if (vtype != OperationValueType::EXP) return "";
                break;
        }

    }
    return ""; // getTextFormula(id);
}

/*
std::string Expressions::getTextFormula (uid_t expressionId)
{
    std::string formula = "#:0:";
    size_t pos, fpos;
    uid_t operationId;
    while ((pos = formula.find("#:")) != std::string::npos) {
        fpos = formula.find(":", pos+2);
        if (fpos == std::string::npos) {
            return "ERROR("+std::to_string(pos)+"):"+formula;
        }
        operationId = stol(formula.substr(pos+2, fpos-pos-2));
        formula.replace(pos, fpos-pos+1, operationId < 5 ?
    }
}*/

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

std::string Expressions::valuesToBinString(uid_t *values, dim_t size, omega_t w)
{
    uint64_t elements[size];
    for (dim_t index = 0; index < size; ++index) {
        elements[index] = Goldilocks::toU64(getEvaluation(values[index], w));
    }
    std::string result;
    result.append((char *)elements, size * 8);
    return result;
}


std::string Expressions::valuesToString(uid_t *values, dim_t size, omega_t w)
{
    std::stringstream ss;
    for (dim_t index = 0; index < size; ++index) {
        if (index) ss << ",";
        ss << Goldilocks::toString(getEvaluation(values[index], w));
    }
    return ss.str();
}

std::string Expressions::valuesBinToString(const std::string &values)
{
    uint size = values.size()/8;
    uint64_t u64values[size];

    memcpy(u64values, values.c_str(), values.size());
    std::stringstream ss;
    for (dim_t index = 0; index < size; ++index) {
        if (index) ss << ",";
        ss << u64values[index];
    }
    return ss.str();
}

void Expressions::expandAlias(void)
{
    uid_t aliasExpressions[count];

    std::cout << "expanding alias ..." << std::endl;
    dim_t aliasCount = 0;
    for (uid_t iexpr = 0; iexpr < count; ++iexpr) {
        if (!expressions[iexpr].isAlias()) continue;
        aliasExpressions[aliasCount++] = iexpr;
    }
    std::cout << "found " << aliasCount << " alias ..." << std::endl;

    uid_t aliasDone = 0;
    #pragma omp parallel for
    for (uid_t ialias = 0; ialias < aliasCount; ++ialias) {
        uid_t id = aliasExpressions[ialias];
        for (omega_t w = 0; w < n; ++w) {
           evaluations[(uint64_t)id * n + w] = expressions[id].getAliasEvaluation(engine, w, GROUP_NONE);
        }
        #pragma omp critical
        {
            ++aliasDone;
            std::cout << "Evaluating expressions " << Tools::percentBar(aliasDone, aliasCount, false) << " " << aliasDone << "/" << aliasCount << "    \t\r" << std::flush;
        }
    }
    std::cout << "expanding alias done." << std::endl;
}

FrElement Expressions::getEvaluation(uid_t id, omega_t w, uid_t evalGroupId)
{
    if (id >= count) {
        throw std::runtime_error("Out of range on expressions accessing id "+std::to_string(id)+" but last id was "+std::to_string(count - 1));
    }

/*    if (expressions[id].isAlias()) {
        return expressions[id].getAliasEvaluation(engine, w, evalGroupId);
    }*/

    if (evalGroupId != GROUP_NONE && expressions[id].groupId != evalGroupId) {
        std::cerr << " evalGroupId:" << evalGroupId << " expressions[" << id << "].groupId:" << expressions[id].groupId << std::endl;
        assert(false);
        exit(1);
    }
    if (checkEvaluated && !expressions[id].evaluated) {
        #pragma omp critical
        std::cerr << " no evaluated expression id:" << id << " evalGroupId:" << evalGroupId << std::endl;
        assert(false);
        exit(1);
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

/*    if (expressions[id].isAlias()) {
        return false;
        throw std::runtime_error("Alias not supported");
    }*/
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
        compileExpression(pilExpressions, index);
        // e.eval(*this);
    }
    // dependencies.dump();
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

void Expressions::updatePercentEvaluated (uint incDone)
{
    #pragma omp critical
    {
        if (incDone) evaluationsDone += incDone;
        std::cout << "Evaluating expressions " << Tools::percentBar(evaluationsDone, count, false) << " " << evaluationsDone << "/" << count << " (cpus:" << activeCpus << ")    \t\r" << std::flush;
    }
}

void Expressions::evalAll(void)
{
    if (evaluations == NULL) {
        std::cout << "Allocating " << Tools::humanSize(count * (uint64_t)n * sizeof(FrElement)) << " for " << count << " expressions" << std::endl;
        evaluations = new FrElement[count * (uint64_t)n];
    }

    deltaW = engine.n / EXPRESSION_EVAL_CHUNKS;
    std::cout << "deltaW: " << deltaW << std::endl;

    if (!expressionChunks) {
        expressionChunks = new ExpressionChunk[EXPRESSION_EVAL_CHUNKS * count];
    }

    uint64_t cpuTimes[cpus];
    evaluationsDone = 0;
    activeCpus = cpus;
    evalDependenciesIndex = 0;
    allExpressionsEvaluated = false;
    updatePercentEvaluated();

    uint64_t startT = Tools::startCrono();
    #pragma omp parallel for
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        evalAllCpuGroup(icpu);
        cpuTimes[icpu] = Tools::endCrono(startT);
        #pragma omp critical
        {
            --activeCpus;
        }
    }


    uint64_t maxTime = 0;
    uint64_t totalTime = 0;
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        if (cpuTimes[icpu] > maxTime) maxTime = cpuTimes[icpu];
        totalTime += cpuTimes[icpu];
    }

    std::cout << std::endl << std::flush;
    std::cout << "--- CPU STATISTICS ---" << std::endl;
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        printf("CPU %3lu [G:%4lu] %s\n", icpu, cpuGroups[icpu].size(), Tools::percentBar(cpuTimes[icpu], maxTime).c_str());
    }
    printf("CPU usage: %0.2f%%  ms: %lu  goal: %lu\n", ((double)totalTime * 100.0)/((double)maxTime * cpus), maxTime, totalTime/cpus);
    std::cout << "CPU usage: --- CPU STATISTICS ---" << std::endl;
    expandAlias();
}

void Expressions::afterEvaluationsLoaded (void)
{
    #pragma omp parallel for
    for (uid_t iexpr = 0; iexpr < count; ++iexpr) {
        // if (isAlias(iexpr)) continue;
        expressions[iexpr].setIsZeroFlag(true);
        expressions[iexpr].setEvaluatedFlag(true);
        omega_t w;
        for (w = 0; w < n; ++w) {
            if (Goldilocks::isZero(getEvaluation(iexpr, w))) continue;
            expressions[iexpr].setIsZeroFlag(false, w);
            break;
        }
        /*
        #pragma omp critical
        if (iexpr == 682 || iexpr == 689 || iexpr == 694 || iexpr == 695 || iexpr == 696 || iexpr == 697) {
            std::cout << "E" << iexpr << ";W" << w << std::endl;
            for (w = 0; w < n; ++w) {
                std::cout << "E" << iexpr << ";" << w << ";" << Goldilocks::toString(getEvaluation(iexpr, w)) << std::endl;
            }
        }*/
    }
}

bool Expressions::hasPendingDependencies (uid_t iexpr) const
{
    return !expressions[iexpr].dependencies.areEvaluated(*this);
}

bool Expressions::hasNextDependency (uid_t iexpr) const
{
    return expressions[iexpr].nextExpression;
}

void Expressions::markChunkAs (uid_t icpu, uid_t iexpr, omega_t fromW, ExpressionChunkState state)
{
    omega_t chunkFromW = 0;
    uid_t chuckStart = EXPRESSION_EVAL_CHUNKS * iexpr;
    uid_t chunkEnd = chuckStart + EXPRESSION_EVAL_CHUNKS - 1;
    uint evaluating = 0;
    uint evaluated = 0;

    for (uid_t chunk = chuckStart; chunk <= chunkEnd; ++chunk) {
        if (fromW == chunkFromW) {
            expressionChunks[chunk].state = state;
            expressionChunks[chunk].icpu = icpu;
        }
        switch (expressionChunks[chunk].state) {
            case ExpressionChunkState::pending:
                break;
            case ExpressionChunkState::evaluating:
                ++evaluating;
                break;
            case ExpressionChunkState::evaluated:
                ++evaluated;
                break;
        }
        chunkFromW += deltaW;
    }
    // if all chunks have been evaluated then the expression has been evaluated
    if (state == ExpressionChunkState::evaluated && evaluated == EXPRESSION_EVAL_CHUNKS) {
        expressions[iexpr].setEvaluatedFlag(true);
    }
}

bool Expressions::nextPedingEvalExpression (uid_t icpu, uid_t &iexpr, omega_t &fromW, omega_t &toW, bool currentDone)
{
    uid_t idep;
    uid_t exprIndex;
    bool expressionFound = false;
    uid_t totalEvaluations = EXPRESSION_EVAL_CHUNKS * count;
    uint iloop = 0;
    omega_t w;
    // #pragma omp critical
    // std::cout << "[" << icpu << "] nextPedingEvalExpression " << evalDependenciesIndex << std::endl;
    while (!expressionFound) {
        ++iloop;
        #pragma omp critical
        {
            if (currentDone) {
                markChunkAs(icpu, iexpr, fromW, ExpressionChunkState::evaluated);
                evaluationsDone += 1;
                std::cout << "Evaluating expressions " << Tools::percentBar(evaluationsDone, totalEvaluations, true) << " " << (uint)(evaluationsDone/EXPRESSION_EVAL_CHUNKS) << "/" << count << " (cpus:" << activeCpus << ")    \t\r" << std::flush;
                currentDone = false;
                // std::cout << "EVAL DONE CPU[" << icpu << "] " << exprIndex << std::endl;
            }
            bool finalLoop = (evalDependenciesIndex == 0);
            bool previousChunksEvaluated = true;
            uint chunkIndex = 0;
            while (!expressionFound && !allExpressionsEvaluated) {
                if (evalDependenciesIndex >= dependencies.size()) {
                    // if make a complete loop without pending elements, there aren't more pending elements
                    // means all expressions was evaluated or are evaluating (assigned)
                    if (finalLoop) {
                        allExpressionsEvaluated = true;
                        break;
                    }
                    finalLoop = true;
                    evalDependenciesIndex = 0;
                    break;
                }
                while (evalDependenciesIndex < dependencies.size()) {
                    // TODO: check if has pendingDependencies or no other cpus evaluating, to
                    // get best performance use same cpu.
                    if (hasPendingDependencies(dependencies[evalDependenciesIndex])) {
                        chunkIndex = 0;
                        ++evalDependenciesIndex;
                    }
                    // calculate current values before increase (evalDependenciesIndex, chunkIndex)
                    if (chunkIndex == 0) {
                        previousChunksEvaluated = true;
                    }
                    w = deltaW * chunkIndex;
                    idep = evalDependenciesIndex;
                    exprIndex = dependencies[idep];
                    auto state = expressionChunks[exprIndex * EXPRESSION_EVAL_CHUNKS + chunkIndex].state;

                    // if last chunkIndex, restart cycle, reset chunkIndex and increment evalDependenciesIndex
                    if (chunkIndex < (EXPRESSION_EVAL_CHUNKS - 1) && (!hasNextDependency(exprIndex) || previousChunksEvaluated)) {
                        ++chunkIndex;
                    } else {
                        chunkIndex = 0;
                        ++evalDependenciesIndex;
                    }
                    // if state was evaluated or evaluating, it can't assigned.
                    if (state == ExpressionChunkState::evaluated) continue;

                    previousChunksEvaluated = false;
                    if (state == ExpressionChunkState::evaluating) continue;

                    // if not evaluated or evaluating means pending, and this isn't final
                    // lastLoop
                    finalLoop = false;
                    if (hasPendingDependencies(exprIndex)) {
                        // only increase evalDependeciesIndex if not increased before.
                        if (idep == evalDependenciesIndex) {
                            chunkIndex = 0;
                            ++evalDependenciesIndex;
                        }
                    } else {
                        markChunkAs(icpu, exprIndex, w, ExpressionChunkState::evaluating);
                        expressionFound = true;
                        break;
                    }
                }
            }
            // if (expressionFound) std::cout << "EVAL START CPU[" << icpu << "] " << exprIndex << std::endl;
        }
        if (allExpressionsEvaluated) return false;
        if (!expressionFound && iloop > 1) return sleep(1);
    }
    fromW = w;
    toW = w + deltaW -1;
    iexpr = exprIndex;
    return true;
}

void Expressions::evalAllCpuGroup (uid_t icpu)
{
    omega_t fromW = 0;
    omega_t toW = 0;
    uid_t iexpr = 0;
    bool currentDone = false;

    while (nextPedingEvalExpression(icpu, iexpr, fromW, toW, currentDone)) {
        #pragma omp critical
        std::cout << "[" << icpu << "] START " << iexpr << " (" << fromW << ".." << toW << ")" << std::endl;
        currentDone = true;
        for (omega_t w = fromW; w <= toW; ++w) {
            evaluations[ (uint64_t)n * iexpr + w ] = expressions[iexpr].eval(engine, w);
        }
        #pragma omp critical
        std::cout << "[" << icpu << "] END " << iexpr << " (" << fromW << ".." << toW << ")" << std::endl;
    }
    /*
    dim_t depCount = dependencies.size();
    for (uint idep = 0; idep < depCount; ++idep) {
        const uid_t iexpr = dependencies[idep];
        if (std::find(cpuGroups[icpu].begin(), cpuGroups[icpu].end(), expressions[iexpr].groupId) == cpuGroups[icpu].end()) continue;
        for (omega_t w = 0; w < n; ++w) {
            evaluations[ (uint64_t)n * iexpr + w ] = expressions[iexpr].eval(engine, w, iexpr == 222288);
        }
        expressions[iexpr].evaluated = true;
        updatePercentEvaluated(1);
        #ifdef __DEBUG__
        // std::cout << "expression[" << iexpr << "] evaluated" << std::endl;
        #endif
    }
    */
}

Expressions::Expressions (Engine &engine)
    :engine(engine)
{
    evaluations = NULL;
    expressions = NULL;
    externalEvaluations = false;
    checkEvaluated = true;
    cpus = omp_get_max_threads();
    cpuGroups = new std::list<uid_t>[cpus];
    expressionChunks = NULL;
}

Expressions::~Expressions (void)
{
    if (!externalEvaluations && evaluations) delete [] evaluations;
    if (expressions) delete [] expressions;
    if (expressionChunks) delete [] expressionChunks;
}

void Expressions::setEvaluations ( FrElement *data )
{
    evaluations = data;
    externalEvaluations = true;
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

    dim_t *cpuGroupSizes = new dim_t[cpus]();
    dim_t cpuGroupSizeTarget = (count / cpus) + ((count % cpus) != 0);
    auto it = groupsBySizeDesc.begin();

    // TODO: if no next expressions => split in n parts the big groups
    dim_t groupSize;
    for (uint loop = 0; it != groupsBySizeDesc.end(); ++loop) {
        for (uint icpu = 0; (icpu < cpus) && it != groupsBySizeDesc.end(); ++icpu) {
            if (cpuGroupSizes[icpu] >= cpuGroupSizeTarget) continue;
            groupId = *it;
            groupSize = groupCounters[groupId];
            cpuGroupSizes[icpu] += groupSize;
            cpuGroups[icpu].push_back(groupId);
            ++it;
        }
    }
    uint totalGroups = 0;
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        totalGroups += cpuGroups[icpu].size();
    }
    std::cout << "Calculated dependencies groups (cpus:" << cpus << ")" << std::endl;
    std::cout << "Groups(we): " << groupsWithElements << "  cpuGroupSizeTarget: " << cpuGroupSizeTarget << "  totalGroups: " << totalGroups << std::endl;

    delete [] groupCounters;
    delete [] cpuGroupSizes;
}


}