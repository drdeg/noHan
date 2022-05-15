#include "ObisListener.h"

namespace esphome {
namespace han {

ObisListener::ObisListener(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6)
{
    _code[0] = b1;
    _code[1] = b2;
    _code[2] = b3;
    _code[3] = b4;
    _code[4] = b5;
    _code[5] = b6;
}

bool ObisListener::matches(const uint8_t *data) const
{
    for (int k = 0; k < 6; k++)
    {
        if (_code[k] != data[k+4])
        {
            return false;
        }
    }
    return true;
}

}   // namespace han
}   // namespace esphome
