#ifndef __PIL__OPERATION_VALUE__HPP__
#define __PIL__OPERATION_VALUE__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>

namespace pil {
    class Engine;
}

#include "fr_element.hpp"
#include "types.hpp"
// #include "engine.hpp"

namespace pil {

enum class OperationValueType { NONE, CM, CONST, EXP, NUMBER, PUBLIC, OP };

class OperationValue
{
    public:
        static const uint DEP_NONE = 0x7FFFFFFF;
        static const char *typeLabels[];

        OperationValueType type;
        uint next;
        union {
            uint64_t u64;
            uid_t id;
            FrElement f;
        } value;
        uid_t set(OperationValueType vType, nlohmann::json& node);
        bool isNextExpression (void) { return next > 0 && type == OperationValueType::EXP; };
        FrElement eval(Engine &engine, omega_t w, uid_t evalGroupId, bool debug = false);
        OperationValue ( void ) { type = OperationValueType::NONE; value.u64 = 0; next = 0; };
};

}

std::ostream& operator << (std::ostream& os, const pil::OperationValueType &obj);

#endif