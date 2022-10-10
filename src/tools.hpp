#ifndef __PIL__TOOLS_HPP__
#define __PIL__TOOLS__HPP__

#include <stdint.h>
#include <sys/time.h>
#include <ctime>
#include <iostream>
#include <string>

namespace pil {

class Tools {
    public:
        static time_t startCrono (void) {
            struct timeval time_now;
            gettimeofday(&time_now, nullptr);
            return (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
        }

        static time_t endCrono (time_t startT) {
            struct timeval time_now;
            gettimeofday(&time_now, nullptr);
            time_t endT = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
            return (endT - startT);
        };

        static void endCronoAndShowIt (time_t startT) {
            time_t ms = endCrono(startT);
            std::cout << "time(ms):" << ms << std::endl;
        };
        static std::string percentBar (uint64_t n, uint64_t total = 100, bool percent = true, const std::string &ansi = "\x1B[1;32m");
};

}

#endif