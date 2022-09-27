#include "expression.hpp"
#include "operation.hpp"

namespace pil {

void Expression::precompile(nlohmann::json& node, int destination, OperationType opType )
{
    OperationValueType vType;

    std::cout << "PRECOMPILE[" << destination << "," << opType << "]: " << node << std::endl;
    if (opType == OperationType::NONE) {
        Operation::decodeType(node["op"], opType, vType);
    }

    if (opType != OperationType::NONE) {
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
                precompile(nvalue, valueDestination, opType);
                continue;
            }
            operations[destination].values[valueIndex].set(vType, nvalue);
        }
        return;
    }
    operations[destination].values[0].set(vType, node);
}


void Expression::dump(void)
{
    for (int index = 0; index < opIndex; ++index) {
        printf("%2d|%-6s(%d)|%-6s(%d)|0x%016luX|%-6s(%d)|0x%016luX|0x%016luX\n", index, Operation::typeLabels[(int)operations[index].op], (int)operations[index].op,
                OperationValue::typeLabels[(int)operations[index].values[0].type], (int)operations[index].values[0].type, operations[index].values[0].value.u64,
                OperationValue::typeLabels[(int)operations[index].values[1].type], (int)operations[index].values[1].type, operations[index].values[1].value.u64,
                Goldilocks::toU64(operations[index].result));
    }
}

void Expression::eval(Engine &engine)
{
    for (int index = opIndex - 1; index >= 0; --index) {
        FrElement values[2];

        values[0] = operations[index].values[0].eval(engine);
    }
}

}