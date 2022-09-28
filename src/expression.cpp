#include <list>
#include <string>
#include "expression.hpp"
#include "operation.hpp"

namespace pil {

Expression::Expression (void)
{
    alias = false;
    compiled = false;
    id = 0;
    next = 0;
    aliasEvaluations = NULL;
}

void Expression::compile(nlohmann::json& node, Dependencies &dependencies)
{
    // std::cout << "compiling ... " << node["id"] << " compiled:" << compiled << std::endl;
    if (compiled) return;

    OperationValueType vType;
    OperationType opType;

    Operation::decodeType(node["op"], opType, vType);

    getFreeId();
    if (opType != OperationType::NONE) {
        recursiveCompile(node, 0, opType, dependencies);
    } else {
        // its an alias
        alias = true;
        next = (node.contains("next") && node["next"]);
        if (next) std::cout << "NEXT ALIAS FOUND" << std::endl;
        const depid_t dependency = operations[0].values[0].set(vType, node);
        if (dependency) {
            dependencies.add(dependency);
        }
    }
    compiled = true;
}

void Expression::recursiveCompile(nlohmann::json& node, dim_t destination, OperationType opType, Dependencies &dependencies)
{
    OperationValueType vType;
/*    if (opIndex <= destination) {
        opIndex = destination + 1;
        if (opIndex >= 16) throw std::runtime_error("pasado de vueltas 1");
    }*/
    std::cout << "recursiveCompile #1 " << "D:" << destination << std::endl;
    if (destination >= operations.size()) {
        throw std::runtime_error("ARRRGG");
    }
    operations[destination].op = opType;
    std::cout << "recursiveCompile #2 " << "D:" << destination << std::endl;

    const int valueCount = (opType == OperationType::NEG) ? 1:2;
    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
        std::cout << "recursiveCompile #3." << valueIndex <<  " D:" << destination <<  "S:" << operations.size() << std::endl;
        auto nvalue = node["values"][valueIndex];
        std::cout << "values[" << valueIndex << "]:" << nvalue << std::endl;
        std::cout << "recursiveCompile #4." << valueIndex <<  " " << nvalue <<  "S:" << operations.size() << std::endl;
        Operation::decodeType(nvalue["op"], opType, vType);
        std::cout << "recursiveCompile #4." << valueIndex <<  " " << nvalue["op"] <<  "S:" << operations.size() << std::endl;
        if (opType != OperationType::NONE) {
            int valueDestination = getFreeId();
            std::cout << "recursiveCompile #5." << valueIndex <<  " VD: " << valueDestination <<  "S:" << operations.size() <<  std::endl;
            operations[destination].values[valueIndex].type = OperationValueType::OP;
            operations[destination].values[valueIndex].value.u64 = valueDestination;
            std::cout << "recursiveCompile #6." << valueIndex <<  " VD:" << valueDestination <<  "S:" << operations.size() <<  std::endl;
            recursiveCompile(nvalue, valueDestination, opType, dependencies);
            continue;
        }
        std::cout << "recursiveCompile #7." << valueIndex <<  " D:" << destination << "S:" << operations.size() << std::endl;
        const uint64_t dependency = operations[destination].values[valueIndex].set(vType, nvalue);
        std::cout << "recursiveCompile #8." << valueIndex <<  " dependency:" << dependency << std::endl;
        if (dependency) {
            dependencies.add(dependency);
        }
        std::cout << "recursiveCompile #9." << valueIndex <<  " dependency:" << dependency << std::endl;
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

void Expression::eval(Engine &engine)
{
    if (alias || !operations.size()) return;
    for (int index = operations.size() - 1; index >= 0; --index) {
        FrElement values[2];

        const dim_t valueIndexCount = (operations[index].op == OperationType::NEG) ? 1:2;
        for (dim_t valueIndex = 0; valueIndex < valueIndexCount; ++valueIndex ) {
            if (operations[index].values[valueIndex].type == OperationValueType::OP) {
                values[valueIndex] = operations[operations[index].values[valueIndex].value.id].result;
            } else {
                values[valueIndex] = operations[index].values[valueIndex].eval(engine);
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
    // dump();
}

}