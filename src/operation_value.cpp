#include "debug.hpp"
#include "operation_value.hpp"
#include "engine.hpp"
#include "fr_element.hpp"

namespace pil {

const char *OperationValue::typeLabels[] = {"none", "cm", "const", "expr", "number", "public", "op"};

uid_t OperationValue::set(OperationValueType vType, nlohmann::json& node)
{
    type = vType;
    if (type == OperationValueType::CONST ||
        type == OperationValueType::CM ||
        type == OperationValueType::PUBLIC ||
        type == OperationValueType::EXP) {
        value.id = node["id"];
    }
    if (type == OperationValueType::NUMBER) {
        std::string svalue = node["value"];
        uint base = 10;
        if (svalue.substr(0, 2) == "0x") {
            svalue = svalue.substr(2);
            base = 16;
        }
        value.f = Goldilocks::fromString(svalue, base);
    }
    next = (node.contains("next") && node["next"]);
    return (type == OperationValueType::EXP) ? value.id : DEP_NONE;
}


FrElement OperationValue::eval(Engine &engine, omega_t w, uid_t evalGroupId, bool debug)
{
    w = engine.next(w + next);
    switch(type) {
        case OperationValueType::CONST: {
            DEBUG_IF(debug, "  getConst [" << engine.getConstName(value.id) << "] (" << value.id << "," << w << ")");
            return engine.getConst(value.id, w);
        }
        case OperationValueType::CM: {
            DEBUG_IF(debug, "  getCommited [" << engine.getCommitedName(value.id) << "] (" << value.id << "," << w << ")");
            return engine.getCommited(value.id, w);
        }
        case OperationValueType::PUBLIC: {
            DEBUG_IF(debug, "  getPublic [" << engine.getPublicName(value.id) << "] (" << value.id << "," << w << ")");
            return engine.getPublic(value.id, w);
        }
        case OperationValueType::EXP: {
            DEBUG_IF(debug, "  getExpression (" << value.id << "," << w << ")");
            return engine.getExpression(value.id, w, evalGroupId);
        }
        case OperationValueType::NUMBER: {
            DEBUG_IF(debug, "  number " << Goldilocks::toU64(value.f));
            return value.f;
        }
        case OperationValueType::OP:
            throw std::runtime_error("Unexpected operation value type OP");
        default:
            throw std::runtime_error("Unknow operation value type "+std::to_string((uint)type));
    }
    return Goldilocks::zero();
}

}

std::ostream& operator << (std::ostream& os, const pil::OperationValueType &obj)
{
    os << static_cast<std::underlying_type<pil::OperationValueType>::type>(obj);
    return os;
}
