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
        }
        if (vtype != OperationValueType::EXP) return "";
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

std::string Expressions::valuesToBinPseudoB64(uid_t *values, dim_t size, omega_t w)
{
    unsigned char elements[size * 11];
    static const char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;

    uint offset = 0;
    for (dim_t index = 0; index < size; ++index) {
        union {
            uint64_t u64;
            unsigned char bytes[8];
        } value;
        value.u64 = Goldilocks::toU64(getEvaluation(values[index], w));
        elements[offset++] = base64[value.bytes[0] & 0x3F];
        elements[offset++] = base64[value.bytes[1] & 0x3F];
        elements[offset++] = base64[value.bytes[2] & 0x3F];
        elements[offset++] = base64[(value.bytes[0] >> 6) | ((value.bytes[1] >> 4) & 0x0C) | ((value.bytes[2] >> 2) & 0x30)];

        elements[offset++] = base64[value.bytes[3] & 0x3F];
        elements[offset++] = base64[value.bytes[4] & 0x3F];
        elements[offset++] = base64[value.bytes[5] & 0x3F];
        elements[offset++] = base64[(value.bytes[3] >> 6) | ((value.bytes[4] >> 4) & 0x0C) | ((value.bytes[5] >> 2) & 0x30)];

        elements[offset++] = base64[value.bytes[6] & 0x3F];
        elements[offset++] = base64[value.bytes[7] & 0x3F];
        elements[offset++] = base64[(value.bytes[6] >> 6) | ((value.bytes[7] >> 4) & 0x0C)];
    }
    // std::cout << "VALUES(" << size << "):" << elements << std::endl;
    result.append((char *)elements, 11 * size);
    return result;
}

std::string Expressions::valuesToBinHex(uid_t *values, dim_t size, omega_t w)
{
    static const char *hexvalues = "0123456789ABCDEF";
    char elements[size * 16];
    std::string result;
    uint offset = 0;
    for (dim_t index = 0; index < size; ++index) {
        union {
            uint64_t u64;
            unsigned char bytes[8];
        } value;
        value.u64 = Goldilocks::toU64(getEvaluation(values[index], w));
        elements[offset++] = hexvalues[value.bytes[0] >> 4];
        elements[offset++] = hexvalues[value.bytes[0] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[1] >> 4];
        elements[offset++] = hexvalues[value.bytes[1] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[2] >> 4];
        elements[offset++] = hexvalues[value.bytes[2] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[3] >> 4];
        elements[offset++] = hexvalues[value.bytes[3] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[4] >> 4];
        elements[offset++] = hexvalues[value.bytes[4] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[5] >> 4];
        elements[offset++] = hexvalues[value.bytes[5] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[6] >> 4];
        elements[offset++] = hexvalues[value.bytes[6] & 0x0F];
        elements[offset++] = hexvalues[value.bytes[7] >> 4];
        elements[offset++] = hexvalues[value.bytes[7] & 0x0F];
    }
    result.append(elements, 16 * size);
    return result;
}

std::string Expressions::valuesToNine(uid_t *values, dim_t size, omega_t w)
{
    struct __attribute__ ((packed)) {
        uint64_t u64;
        uint8_t bits;
    } elements[size];
    memset(elements, 0, size * 9);

    std::string result;
    uint offset = 0;
    for (dim_t index = 0; index < size; ++index) {
        const uint64_t value = Goldilocks::toU64(getEvaluation(values[index], w));
        elements[index].u64 = value | 0x0101010101010101;
        elements[index].bits = (value & 0x0100000000000000) >> 56 |
                               (value & 0x0001000000000000) >> 42 |
                               (value & 0x0000010000000000) >> 35 |
                               (value & 0x0000000100000000) >> 28 |
                               (value & 0x0000000001000000) >> 21 |
                               (value & 0x0000000000010000) >> 14 |
                               (value & 0x0000000000000100) >> 7 |
                               (value & 0x0000000000000001);
    }
    result.append((char *)elements, 9 * size);
    return result;
}

std::string Expressions::valuesToBinPackZero(uid_t *values, dim_t size, omega_t w)
{
    unsigned char elements[size * 16];
    uint offset = 0;
    for (dim_t index = 0; index < size; ++index) {
        union {
            uint64_t u64;
            unsigned char bytes[8];
        } value;
        value.u64 = Goldilocks::toU64(getEvaluation(values[index], w));
        uint8_t zeromap = 0;
        if (value.bytes[0]) elements[offset++] = value.bytes[0];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[1]) elements[offset++] = value.bytes[1];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[2]) elements[offset++] = value.bytes[2];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[3]) elements[offset++] = value.bytes[3];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[4]) elements[offset++] = value.bytes[4];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[5]) elements[offset++] = value.bytes[5];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[6]) elements[offset++] = value.bytes[6];
        else ++zeromap;
        zeromap = zeromap << 1;

        if (value.bytes[7]) elements[offset++] = value.bytes[7];
        else ++zeromap;

        elements[offset++] = zeromap;
    }
    std::string result;
    result.append((char *)elements, offset);
    return result;
}

std::string Expressions::valuesToBinRaw(uid_t *values, dim_t size, omega_t w)
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

    #pragma omp parallel for
    for (uid_t ialias = 0; ialias < aliasCount; ++ialias) {
        uid_t id = aliasExpressions[ialias];
        for (omega_t w = 0; w < n; ++w) {
           evaluations[(uint64_t)id * n + w] = expressions[id].getAliasEvaluation(engine, w, GROUP_NONE);
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
        std::cout << "creating evaluations " << count * engine.n << "(" << count << "*" << engine.n << ") size:" <<  (count * engine.n * sizeof(FrElement)) << std::endl;
        evaluations = new FrElement[count * (uint64_t)n];
    }
    struct timeval time_now;
    gettimeofday(&time_now, nullptr);
    time_t startT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

    // std::cout << "omp_get_max_threads: " << omp_get_max_threads() << std::endl;
    uint64_t done = 0;
    #pragma omp parallel for
    for (uint icpu = 0; icpu < cpus; ++icpu) {
        evalAllCpuGroup(icpu, done);
    }
    expandAlias();

    gettimeofday(&time_now, nullptr);
    time_t endT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
    std::cout << "time(ms):" <<  (endT - startT) << std::endl;
}

void Expressions::afterEvaluationsLoaded (void)
{
    #pragma omp parallel for
    for (uid_t iexpr = 0; iexpr < count; ++iexpr) {
        // if (isAlias(iexpr)) continue;
        expressions[iexpr].setIsZeroFlag(true);
        expressions[iexpr].setEvaluatedFlag(true);
        for (omega_t w = 0; w < n; ++w) {
            if (Goldilocks::isZero(getEvaluation(iexpr, w))) continue;
            expressions[iexpr].setIsZeroFlag(false, w);
            break;
        }
    }
}

void Expressions::evalAllCpuGroup (uid_t icpu, uint64_t &done)
{
    dim_t depCount = dependencies.size();
    for (int idep = 0; idep < depCount; ++idep) {
        const uid_t iexpr = dependencies[idep];
        if (std::find(cpuGroups[icpu].begin(), cpuGroups[icpu].end(), expressions[iexpr].groupId) == cpuGroups[icpu].end()) continue;

        #pragma omp critical
        {
            std::cout << "[" << icpu <<"] CPU (E:" << iexpr << ") " << idep << "/" << depCount << std::endl;
        }

        // for (omega_t w = 0; w < engine.n; ++w) {
        for (omega_t w = 0; w < n; ++w) {
            evaluations[ (uint64_t)n * iexpr + w ] = expressions[iexpr].eval(engine, w, iexpr == 222288);
        }
        expressions[iexpr].evaluated = true;

        #ifdef __DEBUG__
        // std::cout << "expression[" << iexpr << "] evaluated" << std::endl;
        #endif
    }
}

Expressions::Expressions (Engine &engine)
    :engine(engine)
{
    evaluations = NULL;
    expressions = NULL;
    externalEvaluations = false;
    checkEvaluated = true;
    cpus = 64;
    cpuGroups = new std::list<uid_t>[cpus];
}

Expressions::~Expressions (void)
{
    if (!externalEvaluations && evaluations) delete [] evaluations;
    if (expressions) delete [] expressions;
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
            cpuGroupSizes[icpu] += groupSize;
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