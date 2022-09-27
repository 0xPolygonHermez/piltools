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
    FrElement result;
    switch(type) {
        case OperationValueType::CONST:
            break;
        case OperationValueType::CM:
            break;
        case OperationValueType::PUBLIC:
            break;
        case OperationValueType::EXP:
            break;
        case OperationValueType::NUMBER:
            break;
        case OperationValueType::OP:
            break;
        default:
            break;
    }
    return result;
}
}



std::ostream& operator << (std::ostream& os, const pil::OperationValueType &obj)
{
   os << static_cast<std::underlying_type<pil::OperationValueType>::type>(obj);
   return os;
}
