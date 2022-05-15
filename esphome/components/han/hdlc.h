#pragma once

#include <cstdint>

namespace esphome {
namespace han {

/**

## HDLC

HDLC is described in chapter 8.4 here:
https://github.com/roarfred/AmsToMqttBridge/blob/master/Documentation/Excerpt_GB8.pdf

### HDLC LLC Data frame

The LLC PDU is
 - 8 bits: Destination LSAP (0xE6) 
 - 8 bits: Source LSAP (0xE6 or 0xE7)
 - 8 bits: LLC quality (must be 0x00)
 - n*8 bits: information (SDU)

### HDLC MAC Data frame

The MAC PDU is
 - 8 bits: Flag (0x7E) (Omitted if another MAC PDU was just sent)
 - 2 bytes: Frame format. Contains three sub-fields:
    - 4 bit (MSB) Format type (binary: 1010)
    - 1 bit segmentation bit 
    - 11 bit Frame length (excludes the flags)
 - 1-4 bytes: Dest. address
 - 1-4 bytes: Src. address
 - 1 byte: Control field 
 - 2 bytes: HCS
 - n bytes: SDU
 - 2 bytes: FCS
 - 1 byte : Flag (0x7E)

### MAC layer address

The length of the MAC address fields can be any number of octets. The LSB in each octets indicates
if it is the last one in the field.

The dest. address is always only one byte.

The server address is either 1, 2 or 4 bytes. In the multi-byte cases, the first half of the address
is the "upper" HDLC addres and the second half is the "lower" HDLC address.

## SAMPLE
```
7e a243 41 0883 13 85eb e6e700
 0f 40000000 00
 011b
 0202 0906 0000010000ff 090c 07e30c1001073b28ff8000ff
 0203 0906 0100010700ff 06 00000462 0202 0f00 161b
```

MAC PDU header (UP to HCS)
7e: Flag
a243: Frame format: 1010 0010 0100 0011
    0xa: Frame format
    0x0: Segmentation flag 0
    0x234: Frame length 579 dec
41: Dest addres: 0x20
0883: Source address
13: Control field 0001 0011 (unknown what this means)
85eb: HCS (CRC-16 of previous bytes except flag)

The follows the MAC LLC PDU header
e6e700 Destination (e6) and source (e7) LSAPs followed by the LLC quality (0x00)

Then we have the MAC LLC SDU, i.e. the DLMS PDU:
0f 40000000 00 

*/

static const int HDLC_BUFFER_SIZE = 2048;
static const int HDLC_MAC_FOOTER_SIZE = 2;      // This is only the FCS field
static const int HDLC_LLC_HEADER_SIZE = 3;

class HDLCDecoder
{
public:
    HDLCDecoder();

    /// Add a byte to the buffer
    /// Returns true if the frame is in the buffer
    bool push(uint8_t input);

    /// Returns a pointer to the frame data (SDU)
    /// Returns nullptr if header is not received yet
    const uint8_t * getData(void) const;

    /// Returns the length of the frame excluding the start and stop flags
    /// Returns 0 if the header is not received
    int16_t getFrameLength(void) const { return _frameLength; }
    
    /// Get the length of the SDU or 0 if header not received
    int16_t getDataLength(void) const;

    /// Get the size of the header or zero if header not received
    /// Size excludes the leading start flag
    //int16_t getHeaderSize(void) const { return _macHeaderSize == 0 ? 0 : _macHeaderSize - 1 + HDLC_LLC_HEADER_SIZE; }

private:

    uint8_t _buffer[HDLC_BUFFER_SIZE];
    int16_t _bytesInBuffer;         // Number of bytes currently in the buffer
    int16_t _frameLength;           // Length of the frame excluding the start and stop flags (read from MAC header)
    uint8_t _sourceAddressLength;   // Number of bytes in the source addres
    uint8_t _destAddressLength;     // Number of bytes in the destination address
    uint8_t _macHeaderSize;         // Number of bytes in the MAC header, excluding the start flag
    bool _clearOnNext;          //< Flag that indicates if the buffers should be cleared on next input
    const uint8_t _flag = 0x7E;
    
    void clearBuffer();

    bool isValidFrameFormatType(uint8_t frameFormatType)
    {
        // Only the upper 4 bits are the format Type
        //return frameFormatType == 0xA0;
        return (frameFormatType & 0xF0) == 0xA0;
    }
};

}   // namespace han
}   // namespace esphome
