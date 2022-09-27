#include "operation_value.hpp"

namespace pil {

const char *OperationValue::typeLabels[] = {"none", "cm", "const", "expr", "number", "public", "op"};

void OperationValue::set(OperationValueType vType, nlohmann::json& node)
{
    type = vType;
    if (type == OperationValueType::CONST ||
        type == OperationValueType::CM ||
        type == OperationValueType::PUBLIC ||
        type == OperationValueType::EXP) {
        value.u64 = node["id"];
    }
    if (type == OperationValueType::NUMBER) {
        std::string svalue = node["value"];
        value.u64 = strtoull(svalue.c_str(), NULL, 10);
    }
    /* if (type == PilExpressionOperationValueType::OP) {
        value.u64 = node["id"];
    )*/
}

FrElement OperationValue::eval(Engine &engine, int64_t w)
{
    switch(type) {
        case OperationValueType::CONST:
            return engine.getConst(value.u64, w);
        case OperationValueType::CM:
            return engine.getCommited(value.u64, w);
        case OperationValueType::PUBLIC:
            return engine.getPublic(value.u64, w);
        case OperationValueType::EXP:
            break;
        case OperationValueType::NUMBER:
            return value.f;
        case OperationValueType::OP:
            return Goldilocks::zero();
        default:
            return Goldilocks::zero();
    }
    return Goldilocks::zero();
}
}



std::ostream& operator << (std::ostream& os, const pil::OperationValueType &obj)
{
   os << static_cast<std::underlying_type<pil::OperationValueType>::type>(obj);
   return os;
}
