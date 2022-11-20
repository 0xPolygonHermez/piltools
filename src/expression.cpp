#include <list>
#include <string>
#include "expression.hpp"
#include "operation.hpp"
#include "engine.hpp"
#include "goldilocks_base_field.hpp"

namespace pil {

Expression::Expression (void)
{
    alias = false;
    compiled = false;
    evaluated = false;
    id = 0;
    next = 0;
    aliasEvaluations = NULL;
    isZero = true;
    nextExpression = false;
    firstNonZeroEvaluation = 0;
    evaluating = false;
    icpu = false;
}

OperationValueType Expression::getAliasType (void) const
{
    if (!alias) return OperationValueType::NONE;
    return operations[0].values[0].type;
}

FrElement Expression::getAliasEvaluation (Engine &engine, omega_t w, uid_t evalGroupId, bool debug)
{
    assert(alias);
    return operations[0].values[0].eval(engine, w, groupId, debug);
}

FrElement Expression::getAliasValue (void) const
{
    assert(alias);
    return operations[0].values[0].value.f;
}

uint64_t Expression::getAliasValueU64 (void) const
{
    assert(alias);
    return operations[0].values[0].value.u64;
}

bool Expression::compile (nlohmann::json &node)
{
    if (compiled) return false;

    OperationValueType vType;
    OperationType opType;

    Operation::decodeType(node["op"], opType, vType);

    getFreeId();
    if (opType != OperationType::NONE) {
        recursiveCompile(node, 0, opType);
    } else {
        // its an alias
        alias = true;
        next = (node.contains("next") && node["next"]);

        const uid_t dependency = operations[0].values[0].set(vType, node);
        if (dependency != OperationValue::DEP_NONE) {
            dependencies.add(dependency);
        }
        if (next && vType == OperationValueType::EXP) {
            // TODO: review how impacts next expressions on parallel omega evaluations
            throw std::runtime_error("next values with expression not allowed");
        }
    }
    compiled = true;
    return true;
}

void Expression::recursiveCompile(nlohmann::json& node, dim_t destination, OperationType opType)
{
    OperationValueType vType;
    if (destination >= operations.size()) {
        throw std::runtime_error("Destination out of range destination:"+std::to_string(destination)+" size:"+std::to_string(operations.size()));
    }
    operations[destination].op = opType;

    const int valueCount = (opType == OperationType::NEG) ? 1:2;
    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
        auto nvalue = node["values"][valueIndex];
        Operation::decodeType(nvalue["op"], opType, vType);

        #ifdef __DEBUG__
        std::cout << "valueIndex:" << valueIndex << " op:" << nvalue["op"] << " opType:" << opType << " vType:" << vType << std::endl;
        #endif

        if (opType != OperationType::NONE) {
            int valueDestination = getFreeId();
            operations[destination].values[valueIndex].type = OperationValueType::OP;
            operations[destination].values[valueIndex].value.u64 = valueDestination;
            recursiveCompile(nvalue, valueDestination, opType);
            continue;
        }
        const uint64_t dependency = operations[destination].values[valueIndex].set(vType, nvalue);
        if (dependency != OperationValue::DEP_NONE) {
            #ifdef __DEBUG__
            std::cout << "valueIndex:" << valueIndex << " dependency:" << dependency << " id:" << id << " vType:" << vType << std::endl;
            #endif
            dependencies.add(dependency);
        }
        if (operations[destination].values[valueIndex].isNextExpression()) {
            std::cout << "FOUND nextExpression id:" << id << " destination:" << destination << " valueIndex:" <<  valueIndex << std::endl;
            nextExpression = true;
        }
    }
}

void Expression::dump(void) const
{
    for (uint index = 0; index < operations.size(); ++index) {
        printf("%2lu|%02lX|%-6s(%d)|%-6s(%d)|0x%016lX|%-6s(%d)|0x%016lX|0x%016lX\n", (uint64_t)index, (uint64_t)index, Operation::typeLabels[(int)operations[index].op], (int)operations[index].op,
                OperationValue::typeLabels[(int)operations[index].values[0].type], (int)operations[index].values[0].type, operations[index].values[0].value.u64,
                OperationValue::typeLabels[(int)operations[index].values[1].type], (int)operations[index].values[1].type, operations[index].values[1].value.u64,
                Goldilocks::toU64(operations[index].result));
    }
}

dim_t Expression::getFreeId(void)
{
    dim_t id = operations.size();
    Operation emptyOperation;
    operations.push_back(emptyOperation);
    return id;
}

FrElement Expression::eval(Engine &engine, omega_t w, bool debug)
{
    // TODO: REVIEW.

    if (alias) return Goldilocks::zero();
    for (int index = operations.size() - 1; index >= 0; --index) {
        FrElement values[2];

        #ifdef __DEBUG__
        if (debug) {
            std::cout << "\e[1m=== evaluate operation #" << index << " ===\e[0m" << std::endl;
        }
        #endif

        const dim_t valueIndexCount = (operations[index].op == OperationType::NEG) ? 1:2;
        for (dim_t valueIndex = 0; valueIndex < valueIndexCount; ++valueIndex ) {
            if (operations[index].values[valueIndex].type == OperationValueType::OP) {
                #ifdef __DEBUG__
                if (debug) std::cout << "  operations[" << operations[index].values[valueIndex].value.id << "]" << std::endl;
                #endif
                values[valueIndex] = operations[operations[index].values[valueIndex].value.id].result;
            } else {
                values[valueIndex] = operations[index].values[valueIndex].eval(engine, w, groupId, debug);
            }
            #ifdef __DEBUG__
            if (debug) {
                std::cout << "  values[" << valueIndex << "]:" << Goldilocks::toString(values[valueIndex]) << std::endl;
            }
            #endif
        }

        switch (operations[index].op) {
            case OperationType::ADD:
            case OperationType::ADDC:
                operations[index].result = Goldilocks::add(values[0], values[1]);
                break;
            case OperationType::SUB:
                operations[index].result = Goldilocks::sub(values[0], values[1]);
                break;
            case OperationType::MUL:
            case OperationType::MULC:
                operations[index].result = Goldilocks::mul(values[0], values[1]);
                break;
            case OperationType::NEG:
                operations[index].result = Goldilocks::neg(values[0]);
                break;
            case OperationType::NONE:
                operations[index].result = values[0];
                break;
            default:
                throw std::runtime_error("Invalid Type");
                // throw std::runtime_error(std::string("Invalid Operation Type ") + std::string(operations[index].op) + std::string(" on expression ") + std::string(id));
        }
        #ifdef __DEBUG__
        if (debug) std::cout << " OP[" << index << "]: " << Goldilocks::toString( operations[index].result) << std::endl;
        #endif

        // std::cout << "result:" << Goldilocks::toString(operations[index].result) << std::endl;
    }
    if (isZero && !Goldilocks::isZero(operations[0].result)) {
        isZero = false;
    }
    return operations[0].result;
    // dump();
}

uint Expression::replaceOperationValue(OperationValueType oldValueType, uint64_t oldValue, OperationValueType newValueType, uint64_t newValue)
{
    uint count = 0;
    for (uint index = 0; index < operations.size(); ++index) {
        for (uint ivalue = 0; ivalue < 2; ++ivalue) {
            if (operations[index].values[ivalue].type == oldValueType &&
                operations[index].values[ivalue].value.u64 == oldValue) {
                // std::cout << "EXPR[" << id << "] (" << oldValueType << "," << oldValue << ") ==> (" << newValueType << "," << newValue << ")" << std::endl;
                operations[index].values[ivalue].type = newValueType;
                operations[index].values[ivalue].value.u64 = newValue;
                ++count;
            }
        }
    }
    return count;
}

}