#pragma once

#include <cstdint>
#include <list>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"

#include "hdlc.h"
#include "ObisListener.h"

namespace esphome {
namespace han {

/**

IEC 62056 is the international set of standards corresponding to the DLMS/COSEM specifications

The protocol builds on an OSI standard, where SDU is the 'payload' of an PDU. The protocol stack
is as follows:

   +--------+
   | DLMS   | Application layer DLMS (Decoded by ObisSensor objects and HANDecoder)
   | HDLC   | Data Link layer
   |   LLC  | Logical Link Control Sublayer (Decoded by HANDecoder)
   |   MAC  | Media Access Control Sublayer (Decoded by HDLCDecoder)
   | Serial | Physical layer
   +--------+

The app layer consists of a DLSM/COSEM Data Notification PDU
 - COSEM describes the object model that is very generic
 - OBIS is the object identification system, i.e. the naming system of the objects in COSEM
 - DLMS is the application layer protocol

It is unclear if bit-stuffing is used in the HDLC layer. 

Example of DLMS data:

0f 40000000 00
 011b
 0202 0906 0000010000ff 090c 07e30c1001073b28ff8000ff
 0203 0906 0100010700ff 06 00000462 0202 0f00 161b
 0203 0906 0100020700ff 06 00000000 0202 0f00 161b
 0203 0906 0100030700ff 06 000005e3 0202 0f00 161d
 ...

Decoding:

0f: APDU TAG
40000000: APDU invoke ID and priority
00: Extra header size
011b: COSEM dataSize
Then follows the COSEM frame ("Notification body")

*/

class HANDecoder : public Component, public esphome::uart::UARTDevice, public esphome::sensor::Sensor
{
public:
    HANDecoder();

    void setup() override
    {
    }

    // The loop function is called about 60 times per second
    void loop() override;

    /// Call this method to register a listener
    void registerListener(ObisListener* listener);

private:
    /// List of listeners
    std::list< ObisListener* > _listeners;

    /// HDLC Frame decoder
    HDLCDecoder* _hdlcDecoder = new HDLCDecoder();

    /// Decodes the HAN data in the HDLC data frame
    bool decodeDlms(const uint8_t *buffer, const uint16_t bufferSize);

    /// Get the length of an COSEM element
    int16_t getCosemLength(const uint8_t *buffer, const int16_t bufferSize);

    /// Decodes the COSEM Frame and returns its size
    uint16_t decodeCosemFrame(const uint8_t *buffer, const uint16_t bufferSize);
};


}   // namespace han
}   // namespace esphome
