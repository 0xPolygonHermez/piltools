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


uint64_t Tools::u64Log2 ( uint64_t value )
{
    uint64_t upto = 1;
    uint64_t log = 0;
    while (value > upto) {
        ++log;
        if (upto >= 0x8000000000000000UL) break;
        upto *= 2;
    }
    return log;
}

std::string Tools::humanSize ( uint64_t size )
{
    char output[200];
    double unitSize;
    static const uint64_t dimension[5] = {1000000000000ULL, 1000000000ULL, 1000000ULL, 1000, 1};
    static const char *units[5] = {"TB", "GB", "MB", "KB", "bytes"};
    uint index;
    for (index = 0; index < 4; ++index) {
        if (size > dimension[index]) break;
    }
    unitSize = (double)size / dimension[index];
    if (unitSize > 50 || dimension[index] == 1) {
        snprintf(output, sizeof(output), "%u %s", (uint) unitSize, units[index]);
    }
    else {
        snprintf(output, sizeof(output), "%0.2f %s", unitSize, units[index]);
    }
    return output;
}


std::string Tools::replaceAll (const std::string &value, const std::string& search, const std::string& replace)
{
    std::string result = "";
    size_t ppos = 0;
    size_t pos = 0;
    size_t len = search.size();
    while((pos = value.find(search, ppos)) != std::string::npos) {
        if (pos > ppos) result += value.substr(ppos, pos - ppos);
        result += replace;
        ppos = pos + len;
    }
    result += value.substr(ppos);
    return result;
}

}