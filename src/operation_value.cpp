#include "operation_value.hpp"
#include "engine.hpp"

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
        value.f = Goldilocks::fromU64(strtoull(svalue.c_str(), NULL, 10));
    }
    /* if (type == PilExpressionOperationValueType::OP) {
        value.u64 = node["id"];
    )*/
    return (type == OperationValueType::EXP) ? value.id : 0;
}

FrElement OperationValue::eval(Engine &engine, omega_t w)
{
    switch(type) {
        case OperationValueType::CONST:
            return engine.getConst(value.id, w);
        case OperationValueType::CM:
            return engine.getCommited(value.id, w);
        case OperationValueType::PUBLIC:
            return engine.getPublic(value.id, w);
        case OperationValueType::EXP:
            break;
        case OperationValueType::NUMBER:
            return value.f;
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
