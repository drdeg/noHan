#pragma once
#include <cstdint>
#include "ObisListener.h"

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace han {

class ObisSensor : public esphome::han::ObisListener, public sensor::Sensor, public Component
{
public:
    ObisSensor(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6 ) :
        ObisListener(b1, b2, b3, b4, b5, b6)
    {
    }

    virtual bool decode(const uint8_t* data) override;

protected:

    bool decodeTime(const uint8_t* buffer);

};

}   // namespace han
}   // namespace esphome
