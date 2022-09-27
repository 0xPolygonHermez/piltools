#include "expression.hpp"
#include "operation.hpp"

namespace pil {

void Expression::compile(nlohmann::json& node)
{
    if (compiled) return;

    OperationValueType vType;
    OperationType opType;

    std::cout << "COMPILE: " << node << std::endl;
    Operation::decodeType(node["op"], opType, vType);

    if (opType != OperationType::NONE) {
        recursiveCompile(node, 0, opType);
    } else {
        operations[0].values[0].set(vType, node);
    }
    compiled = true;
}

void Expression::recursiveCompile(nlohmann::json& node, int destination, OperationType opType )
{
    OperationValueType vType;
    if (opIndex <= destination) {
        opIndex = destination + 1;
    }
    operations[destination].op = opType;
    int valueCount = opType == OperationType::NEG ? 1:2;
    for (int valueIndex = 0; valueIndex < valueCount; ++valueIndex) {
        nlohmann::json& nvalue = node["values"][valueIndex];
        std::cout << "values[" << valueIndex << "]:" << nvalue << std::endl;
        Operation::decodeType(nvalue["op"], opType, vType);
        if (opType != OperationType::NONE) {
            int valueDestination = opIndex++;
            operations[destination].values[valueIndex].type = OperationValueType::OP;
            operations[destination].values[valueIndex].value.u64 = valueDestination;
            recursiveCompile(nvalue, valueDestination, opType);
            continue;
        }
        operations[destination].values[valueIndex].set(vType, nvalue);
    }
}

void Expression::dump(void)
{
    for (int index = 0; index < opIndex; ++index) {
        printf("%2d|%-6s(%d)|%-6s(%d)|0x%016lX|%-6s(%d)|0x%016lX|0x%016lX\n", index, Operation::typeLabels[(int)operations[index].op], (int)operations[index].op,
                OperationValue::typeLabels[(int)operations[index].values[0].type], (int)operations[index].values[0].type, operations[index].values[0].value.u64,
                OperationValue::typeLabels[(int)operations[index].values[1].type], (int)operations[index].values[1].type, operations[index].values[1].value.u64,
                Goldilocks::toU64(operations[index].result));
    }
}

void Expression::eval(Engine &engine)
{
    for (int index = opIndex - 1; index >= 0; --index) {
        FrElement values[2];

        const int valueIndexCount = (operations[index].op == OperationType::NEG) ? 1:2;
        for (int valueIndex = 0; valueIndex < valueIndexCount; ++valueIndex ) {
            if (operations[index].values[valueIndex].type == OperationValueType::OP) {
                values[valueIndex] = operations[operations[index].values[valueIndex].value.u64].result;
            } else {
                values[valueIndex] = operations[index].values[valueIndex].eval(engine);
            }
            std::cout << "values[" << valueIndex << "]:" << Goldilocks::toString(values[valueIndex]) << std::endl;
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
                throw "Invalid Operation Type";
        }
        std::cout << "result:" << Goldilocks::toString(operations[index].result) << std::endl;
    }
    dump();
}

}