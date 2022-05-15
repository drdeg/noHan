#pragma once

#include <cstdint>

namespace esphome {
namespace han {

class ObisListener
{
public:
    ObisListener(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6 );

    /// Check if the datablock mathes the listener
    bool matches(const uint8_t* data) const;

    /// Override this function to implement decoding
    virtual bool decode(const uint8_t* data) = 0;

private:
    uint8_t _code[6];
};


}   // namespace han
}   // namespace esphome
