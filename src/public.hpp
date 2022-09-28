#ifndef __PIL__PUBLIC__HPP__
#define __PIL__PUBLIC__HPP__

namespace pil {
    class PublicValue;
    class PublicValues;
}

#include <stdint.h>
#include <string>
#include <map>
#include "fr_element.hpp"

namespace pil {
    class PublicValue {
        public:
            std::string name;
            uid_t id;
            FrElement value;
            PublicValue(void) { id = 0; value = Goldilocks::zero (); }
    };


    class PublicValues {
        public:
            void add(uid_t id, const std::string &name, FrElement value);
            FrElement getValue(uid_t id) { return values[id].value; }
            std::string getName(uid_t id) { return values[id].name; }
            PublicValues(void) {}
        protected:
            std::map<uint, PublicValue> values;
    };
}

#endif