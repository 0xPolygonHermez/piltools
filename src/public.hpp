#ifndef __PIL__PUBLIC__HPP__
#define __PIL__PUBLIC__HPP__

namespace pil {
    class PublicValue;
    class PublicValues;
}

#include <stdint.h>
#include <string>
#include "fr_element.hpp"

namespace pil {
    class PublicValue {
        public:
            char name[64];
            int id;
            FrElement value;
    };


    class PublicValues {
        public:
            void add(int id, const std::string &name, FrElement value);
            FrElement getValue(uint64_t id) { return values[id].value; }
            std::string getName(uint64_t id) { return values[id].name; }
        protected:
            int count = 0;
            int size = 0;
            PublicValue *values = NULL;
    };
}

#endif