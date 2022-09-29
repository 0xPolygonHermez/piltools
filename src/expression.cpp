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
    id = 0;
    next = 0;
    aliasEvaluations = NULL;
}

OperationValueType Expression::getAliasType ( void )
{
    if (!alias) return OperationValueType::NONE;
    return operations[0].values[0].type;
}

FrElement Expression::getAliasValue ( void )
{
    if (!alias) return Goldilocks::zero();
    return operations[0].values[0].value.f;
}

uint64_t Expression::getAliasValueU64 ( void )
{
    if (!alias) return 0;
    return operations[0].values[0].value.u64;
}

void Expression::compile(nlohmann::json& node)
{
    if (compiled) return;

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
        std::cout << "ALIAS: " << vType << node << std::endl;

        const uid_t dependency = operations[0].values[0].set(vType, node);
        if (dependency) {
            dependencies.add(dependency);
        }
        if (next) std::cout << "NEXT ALIAS FOUND" << std::endl;
    }
    compiled = true;
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
        if (opType != OperationType::NONE) {
            int valueDestination = getFreeId();
            operations[destination].values[valueIndex].type = OperationValueType::OP;
            operations[destination].values[valueIndex].value.u64 = valueDestination;
            recursiveCompile(nvalue, valueDestination, opType);
            continue;
        }
        const uint64_t dependency = operations[destination].values[valueIndex].set(vType, nvalue);
        if (dependency) {
            dependencies.add(dependency);
        }
    }
}

void Expression::dump(void)
{
    for (uint index = 0; index < operations.size(); ++index) {
        printf("%2u|%02X|%-6s(%d)|%-6s(%d)|0x%016lX|%-6s(%d)|0x%016lX|0x%016lX\n", index, index, Operation::typeLabels[(int)operations[index].op], (int)operations[index].op,
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

FrElement Expression::eval(Engine &engine, omega_t w)
{
    if (alias) return Goldilocks::zero();
    for (int index = operations.size() - 1; index >= 0; --index) {
        FrElement values[2];

        const dim_t valueIndexCount = (operations[index].op == OperationType::NEG) ? 1:2;
        for (dim_t valueIndex = 0; valueIndex < valueIndexCount; ++valueIndex ) {
            if (operations[index].values[valueIndex].type == OperationValueType::OP) {
                values[valueIndex] = operations[operations[index].values[valueIndex].value.id].result;
            } else {
                values[valueIndex] = operations[index].values[valueIndex].eval(engine, w);
            }
            // std::cout << "values[" << valueIndex << "]:" << Goldilocks::toString(values[valueIndex]) << std::endl;
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
        // std::cout << "result:" << Goldilocks::toString(operations[index].result) << std::endl;
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
                std::cout << "EXPR[" << id << "] (" << oldValueType << "," << oldValue << ") ==> (" << newValueType << "," << newValue << ")" << std::endl;
                operations[index].values[ivalue].type = newValueType;
                operations[index].values[ivalue].value.u64 = newValue;
                ++count;
            }
        }
    }
    return count;
}

FrElement Expression::getEvaluation(omega_t w) const
{
    return Goldilocks::zero();
}

}