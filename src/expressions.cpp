#include <sys/time.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "expressions.hpp"
#include "operation_value.hpp"
#include "operation.hpp"
#include "engine.hpp"

namespace pil {

uint Expressions::reduceAliasExpressions (void)
{
    uint replacements = 0;
    for (auto aliasId = 0; aliasId < count; ++aliasId) {
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
    for (auto index = 0; index < count; ++index) {
        replacements += expressions[index].replaceOperationValue(oldValueType, oldValue, newValueType, newValue);
    }
    return replacements;
}

uint Expressions::reduceNumberAliasExpressions (void)
{
    uint count = 0;
    for (auto aliasId = 0; aliasId < count; ++aliasId) {
        if (expressions[aliasId].getAliasType() != OperationValueType::NUMBER) continue;
        count += replaceOperationValue(OperationValueType::EXP, aliasId, OperationValueType::NUMBER, expressions[aliasId].getAliasValueU64());
    }
    std::cout << count << " replacements" << std::endl;
    return count;
}

void Expressions::calculateDependencies (void)
{
    for (auto index = 0; index < count; ++index) {
        recursiveCalculateDependencies(index);
    }
    std::cout << dependencies.size() << " " << count << std::endl;
}

void Expressions::recursiveCalculateDependencies (uid_t expressionId)
{
    if (dependencies.contains(expressionId)) return;
    Expression &expression = expressions[expressionId];

    for (auto index = 0; index < expression.dependencies.size(); ++index) {
        recursiveCalculateDependencies(expression.dependencies[index]);
    }
    dependencies.add(expressionId);
}

FrElement Expressions::getEvaluation(uid_t id, omega_t w)
{
    if (id >= count) {
        throw std::runtime_error("Out of range on expressions accessing id "+std::to_string(id)+" but last id was "+std::to_string(count - 1));
    }

    if (expressions[id].isAlias()) {
        return expressions[id].getEvaluation(w);
    }
    return evaluations[ w * count + id];
}

void Expressions::loadAndCompile(nlohmann::json &pilExpressions)
{
    count = pilExpressions.size();
    expressions = new Expression[count];

    dim_t index = 0;
    for (auto it = pilExpressions.begin(); it != pilExpressions.end(); ++it) {
        std::cout << "========== compiling " << index << " ==========" << std::endl;
        expressions[index].id = index;
        expressions[index].compile(*it);
        ++index;
        // e.eval(*this);
    }
}

void Expressions::evalAll(Engine &engine)
{
    struct timeval time_now;
    gettimeofday(&time_now, nullptr);
    time_t startT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

    for (uid_t iexpr = 0; iexpr < count; ++iexpr) {
        for (omega_t w = 0; w < 65536; ++w) {
            // std::cout << "========== evaluating " << iexpr << std::endl;
            expressions[iexpr].eval(engine, w);
        }
    }
    gettimeofday(&time_now, nullptr);
    time_t endT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
    std::cout << "time(ms):" <<  (endT - startT) << std::endl;
}

}