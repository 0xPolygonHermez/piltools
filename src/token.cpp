#include <string.h>
#include "token.hpp"

namespace pil {

int Token::indexInList (const std::string &list, const std::string &value, char separator)
{
    const auto valueLen = value.size();
    const char *pvalue = value.c_str();
    const char *endList = list.c_str() + list.size();

    const char *currentP = list.c_str();
    int index = 0;

    do {
        const bool atEnd = endList <= (currentP + valueLen);
        if (strncmp(currentP, pvalue, valueLen) == 0 && ((!atEnd && currentP[valueLen] == separator) || !atEnd)) {
            return index;
        }
        ++index;
        if ((currentP = strchr(currentP, separator)) != NULL) {
            break;
        }
        ++currentP;
    } while (currentP < endList);

    return -1;
}

}