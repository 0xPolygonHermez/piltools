#include "operation.hpp"

namespace pil {

const char *Operation::typeLabels[] = {"none", "add", "sub", "mul", "addc", "mulc", "neg"};

void Operation::decodeType(const std::string &value, OperationType &operationType, OperationValueType &valueType )
{
    static const char *tokens = "add|sub|mul|addc|mulc|neg|cm|const|expr|number|public|";
    static const char *ids =    "1---2---3---4----5----6---A--B-----C----D------E";

    const char *p = strstr(tokens, value.c_str());
    const char v = p ? ids[p - tokens]: 0;
    operationType = (OperationType) ((v && v <= '9') ? (int)(v - '0'): 0);
    valueType = (OperationValueType)((v && v >= 'A') ? ((int)(v - 'A')+1): 0);

    std::cout << "## DECODE ## " << value << " " << operationType << " " << valueType << std::endl;
}

}

std::ostream& operator << (std::ostream& os, const pil::OperationType &obj)
{
   os << static_cast<std::underlying_type<pil::OperationType>::type>(obj);
   return os;
}
