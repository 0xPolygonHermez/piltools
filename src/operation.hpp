#ifndef __PIL__OPERATION__HPP__
#define __PIL__OPERATION__HPP__

namespace pil {
    class Operation;
    enum class OperationType;
}

#include <iostream>
#include <nlohmann/json.hpp>
#include "operation_value.hpp"

namespace pil {

enum class OperationType { NONE, ADD, SUB, MUL, ADDC, MULC, NEG };

class Operation {
    public:
        static const char *typeLabels[];

        OperationType op = OperationType::NONE;
        OperationValue values[2];
        FrElement result;
        Operation(void) { op = OperationType::NONE; Goldilocks::zero(result); }
        static void decodeType(const std::string &value, OperationType &operationType, OperationValueType &valueType );
};

}

std::ostream& operator << (std::ostream& os, const pil::OperationType &obj);

#endif