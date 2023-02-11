#include "hdlc.h"

#include "esphome/core/log.h"
#include "crc.h"
// #define HDLC_DEBUGING

namespace esphome {
namespace han {

HDLCDecoder::HDLCDecoder()
{
    clearBuffer();
}

const uint8_t * HDLCDecoder::getData(void) const
{
    if (getDataLength() > 0)
    {
        // +1 for the flag
        return &_buffer[1 + _macHeaderSize + HDLC_LLC_HEADER_SIZE];
    }
    else
    {
        return nullptr;
    }
}

int16_t HDLCDecoder::getDataLength() const
{
    if (_macHeaderSize > 0)
    {
        // FrameLength excludes the start and stop flags
        return _frameLength - HDLC_MAC_FOOTER_SIZE - HDLC_LLC_HEADER_SIZE - _macHeaderSize;
    }
    else
    {
        return 0;
    }
    
}

void HDLCDecoder::clearBuffer(void)
{
    _bytesInBuffer = 0;
    _frameLength = 0;
    _sourceAddressLength = 0;
    _destAddressLength = 0;
    _macHeaderSize = 0;
    _clearOnNext = false;
    _escape = false;
}

bool HDLCDecoder::push(uint8_t input)
{
    if (_clearOnNext)
    {
        clearBuffer();
    }

    if (_bytesInBuffer == 0 && input != _frameDelimiter)
    {
        // The buffer is emtpy, wait for the next frame delimiter
        return false;
    }
    else
    {
        if (_escape)
        {
            // append the read byte to the buffer
            _buffer[ _bytesInBuffer++ ] = input ^ _escapeMask;
            _escape = false;
        }
        else if ( input == _controlEscapeOctet )
        {
            ESP_LOGD("hdlc", "Received control escape character");
            _escape = true;
            return false;
        }
        else
        {
            // append the read byte to the buffer
            _buffer[ _bytesInBuffer++ ] = input;
        }

#ifdef HDLC_DEBUGING
        if (_macHeaderSize == 0)
        {
            ESP_LOGD("hdlc", "Header byte received: 0x%02X", input);
        }
        else if (_bytesInBuffer >= _frameLength)
        {
            // Should be three bytes (CRC+flag)
            ESP_LOGD("hdlc", "End data received: 0x%02X %d", input, _bytesInBuffer);
        }
#endif

        // The huge decode block
        if (_bytesInBuffer == 1)
        {
            // This was the start flag, need more bytes
            ESP_LOGD("hdlc", "Received start flag 0x%02X", _frameDelimiter);
        }
        else if (_bytesInBuffer == 2)
        {
            if (input == _frameDelimiter)
            {
                // We have received two consecutive flags. This means that the
                // first one was actually the end flag of the previous frame.
                ESP_LOGD("hdlc", "Double start flag 0x%02X", _frameDelimiter);
                _bytesInBuffer = 1;
            }
            else
            {
                // We can now decode the frame type
                uint8_t frameFormatType = (uint8_t)(_buffer[1] & 0xF0);
                if ( !isValidFrameFormatType(frameFormatType) )
                {
                    // Invalid frame format. Clear the buffer
                    ESP_LOGW("hdlc", "Invalid frame format 0x%02X", frameFormatType);
                    clearBuffer();
                }
                else
                {
                    ESP_LOGD("hdlc", "Received frame format %0x%02X", frameFormatType);
                }
            }
        }
        else if (_bytesInBuffer == 3)
        {
            // Capture the length of the HDLC frame
            // Excluding opening and closing frame flags
            _frameLength = ((_buffer[1] & 0x07) << 8) | _buffer[2];
            ESP_LOGD("hdlc", "Data length: %d", _frameLength);
            if (_frameLength + 2 > HDLC_BUFFER_SIZE)
            {
                ESP_LOGW("hdlc", "HDLC frame lager than buffer");
                clearBuffer();
            }
            ESP_LOGD("hdlc", "Received frame data length %d", _frameLength);
        }
        else if (_destAddressLength == 0)
        {
            // Destination address length is not fully received yet
            // LSB == 1 means that this was the last byte in the address
            if ((input & 0x01) == 0x01)
            {
                ESP_LOGD("hdlc", "Dest end flag received");
                _destAddressLength = _bytesInBuffer - 3;
                if (_destAddressLength == 0)
                {
                    // This is very wrong
                    ESP_LOGW("hdlc", "Dest address is null");
                    clearBuffer();
                }
                ESP_LOGD("hdlc", "Destination addres length is %d", _destAddressLength);
            }
            else
            {
                ESP_LOGD("hdlc", "Long dest address");
            }
        }
        else if (_sourceAddressLength == 0)
        {
            // Source address length is not fully received yet
            // LSB == 1 means that this was the last byte in the address
            if ((input & 0x01) == 0x01)
            {
                _sourceAddressLength = _bytesInBuffer - 3 - _destAddressLength;
                if (_sourceAddressLength == 0)
                {
                    // This is very wrong
                    ESP_LOGW("hdlc", "Src address is null");
                    clearBuffer();
                }
                ESP_LOGD("hdlc", "Source addres length is %d", _sourceAddressLength);

                // We can now also compute the header size
                _macHeaderSize = 3 + _sourceAddressLength + _destAddressLength + 2;
                ESP_LOGD("hdlc", "MAC Header size is %d", _macHeaderSize);
            }
            else
            {
                ESP_LOGD("hdlc", "Long source address");
            }

        }
        else if (_bytesInBuffer == (_macHeaderSize+1))
        {
            // The entire MAC header is received, verify the checksum
            uint16_t hcs = (_buffer[_bytesInBuffer-1] << 8) | _buffer[_bytesInBuffer-2];
            ESP_LOGD("hdlc", "HDLC MAC header is received: size is %d", _bytesInBuffer);
            // TODO: Verify the checksum

            // Compute checksum
            uint16_t hcsCalc = computeCRC16(0xffff, &_buffer[1], _bytesInBuffer - 3);
            hcsCalc ^= 0xffff;

            ESP_LOGD("hdlc", "HDLC HCS is calculated to 0x%04X, expected 0x%04X", hcsCalc, hcs);
        }
        else if (_bytesInBuffer == _frameLength + 1)
        {
            // All data is received, verify the checksum
            //uint16_t checkSum = (_buffer[_bytesInBuffer-1] << 8) | _buffer[_bytesInBuffer-2];
            ESP_LOGD("hdlc", "HDLC data is received, next byte should be the frame delimiter.");
        }
        else if (_bytesInBuffer >= _frameLength + 2)
        {
            // This would be the stop flag
            if (input == _frameDelimiter)
            {
                ESP_LOGD("hdlc", "HDLC frame end flag is received, buffer size is %d", _bytesInBuffer);

                uint16_t fcs = (_buffer[_bytesInBuffer-2] << 8) | _buffer[_bytesInBuffer-3];

                // Compute the FCS lenght is len - 4, (0x7E, FCS, and 0x7E)
                uint16_t fcsCalc = computeCRC16(0xffff, &_buffer[1], _bytesInBuffer - 4);
                fcsCalc ^= 0xffff;

                ESP_LOGD("hdlc", "HDLC FCS is calculated to 0x%04X, expected is 0x%04X", fcsCalc, fcs);

                // Start decoding a new message
                _clearOnNext = true;
                if (fcsCalc != fcs)
                {
                    ESP_LOGW("hdlc", "HDLC message failed FCS check. Discarding message.");
                    return false;
                }
                else
                {
                    ESP_LOGI("hdlc", "HDLC message is sane");
                    return true;
                }
            }
            else if (_bytesInBuffer >= _frameLength + 10)
            {
                // Unexpected data
                ESP_LOGW("hdlc", "Giving up waiting for end flag. This means we're out of sync.");
                clearBuffer();
            }
        }
        
        // Debugging
#ifdef HDLC_DEBUGING
        if (input == _frameDelimiter)
        {
            ESP_LOGD("hdlc", "Found flag in stream at pos %d", _bytesInBuffer-1);
        }
#endif
        return false;
    }
}

}   // namespace han
}   // namespace esphome
