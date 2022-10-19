#ifndef __CYCLIC__HPP__
#define __CYCLIC__HPP__

#include <string>
#include <iostream>
#include "types.hpp"

namespace pil
{

    class Cyclic
    {
        public:
            Cyclic (void) { state = INIT; _offset = 0; _period = 0; _last = 0; };
            inline int cycle (omega_t w);
            bool isCyclic (omega_t n = 0) { return state == CYCLIC && (n == 0 || ((_last + _period) % n) == _offset); };
            omega_t period (void) { return _period; };
            omega_t offset (void) { return _offset; };
            omega_t last (void) { return _last; };
        protected:
            enum { INIT, OFFSET_SET, PERIOD_SET, CYCLIC, NO_CYCLIC, INVALID } state;
            omega_t _offset;
            omega_t _period;
            omega_t _last;
    };

    int Cyclic::cycle (omega_t w)
    {
//        std::cout << "ENTER cycle(" << w << ") ST=" << state << " O=" << _offset << " P=" << _period << " L=" << _last << std::endl;
        if (_last >= w) {
//            std::cout << "EXIT cycle(" << w << ") -1 ST=" << state << " O=" << _offset << " P=" << _period << " L=" << _last << std::endl;
            return -1;
        }

        switch (state) {
            case INIT:
                _offset = w;
                state = OFFSET_SET;
                break;

            case OFFSET_SET:
                _period = w - _offset;
                state = PERIOD_SET;
                break;

            case PERIOD_SET:
            case CYCLIC:
                if (w == (_last + _period)) {
//                    std::cout << "WARNING cycle(" << w << ") L+P=" << (_last + _period) << std::endl;

                    state = CYCLIC;
                } else {
                    state = NO_CYCLIC;
                }
                break;

            case INVALID:
                return -1;

            case NO_CYCLIC:
                break;

            default:
                state = INVALID;
//                std::cout << "EXIT cycle(" << w << ") -1* ST=" << state << " O=" << _offset << " P=" << _period << " L=" << _last << std::endl;
                return -1;
        }
        _last = w;
//        std::cout << "EXIT cycle(" << w << ") 0 ST=" << state << " O=" << _offset << " P=" << _period << " L=" << _last << std::endl;
        return 0;
    }
}

#endif