#ifndef __PIL__TOOLS__HPP__
#define __PIL__TOOLS__HPP__

#include <stdint.h>
#include <sys/time.h>
#include <ctime>
#include <iostream>
#include <string>

namespace pil {

class Tools {
    public:
        static uint64_t startCrono (void) {
            struct timeval time_now;
            gettimeofday(&time_now, nullptr);
            return (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
        }

        inline static uint64_t endCrono (uint64_t startT) { return timeCrono(startT); };
        static uint64_t timeCrono (uint64_t startT) {
            struct timeval time_now;
            gettimeofday(&time_now, nullptr);
            time_t endT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
            return (endT - startT);
        };

        inline static uint64_t endCronoAndShowIt (uint64_t startT, const std::string label = "") { return timeCronoAndShowIt(startT, label); };
        static uint64_t timeCronoAndShowIt (uint64_t startT, const std::string label = "") {
            uint64_t ms = timeCrono(startT);
            std::cout << label << "time(ms):" << ms << std::endl;
            return ms;
        };
        static std::string percentBar (uint64_t n, uint64_t total = 100, bool percent = true, const std::string &ansi = "\x1B[1;32m");

        static uint64_t u64Log2 ( uint64_t value );
        static std::string humanSize ( uint64_t size );

        static std::string replaceAll (const std::string &value, const std::string& search, const std::string& replace);
};

}

#endif