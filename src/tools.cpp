#include "tools.hpp"

namespace pil {

std::string Tools::percentBar (uint64_t n, uint64_t total, bool percent, const std::string &ansi)
{
    static const char *barChars = "\u258F\u258E\u258D\u258C\u258B\u258A\u2589\u2588";
    static const char *completeBar = "\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588";
    static const char *pendingBar = "                ";
    static const char *digits = "0123456789";
    std::string res = "[" + ansi;
    uint done = (n << 7)/total;
    uint complete = done >> 3;
    uint partial = done & 0x07;
    uint pending = 16 - complete;
    res += (completeBar + 48 - (complete * 3));
    if (partial) {
        res.append(barChars + (partial-1)*3, 3);
        --pending;
    }
    res += (pendingBar + 16 - pending);
    uint perthousand = n * 1000/total;
    res += "\x1B[0m]";
    if (percent) {
        res += " "+std::to_string(perthousand/10)+"."+digits[perthousand%10]+"%";
    }

    return res;
}

}