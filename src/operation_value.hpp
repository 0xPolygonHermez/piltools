#ifndef __PIL__OPERATION_VALUE__HPP__
#define __PIL__OPERATION_VALUE__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>

namespace pil {
    class OperationValue;
    enum class OperationValueType;
}

#include "fr_element.hpp"
#include "engine.hpp"

namespace pil {

enum class OperationValueType { NONE, CM, CONST, EXP, NUMBER, PUBLIC, OP };

class OperationValue
{
    public:
        static const char *typeLabels[];

        OperationValueType type;
        union {
            uint64_t u64;
            FrElement f;
        } value;
        void set(OperationValueType vType, nlohmann::json& node);
        FrElement eval(Engine &engine, int64_t w = 0);
};

}

std::ostream& operator << (std::ostream& os, const pil::OperationValueType &obj);

#endif