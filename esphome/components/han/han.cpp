#include "han.h"
#include "esphome/core/log.h"

namespace esphome {
namespace han {

HANDecoder::HANDecoder() 
{ 

}

void HANDecoder::registerListener(ObisListener* listener)
{
    if (listener != nullptr)
        this->_listeners.push_back(listener);
}

void HANDecoder::loop(void)
{
    while (available() > 0)
    {
        uint8_t input;
        read_byte(&input);
        if (_hdlcDecoder->push(input))
        {
            // A frame is received
            ESP_LOGI("HDLCReader", "A HDLC frame is received!");

            // Data is the buffer with the start/stop flags and HDLC header removed
            int16_t dataSize = _hdlcDecoder->getDataLength();
            const uint8_t *data = _hdlcDecoder->getData();

            if (data != nullptr && dataSize > 10)
            {
                decodeDlms(data, dataSize);
            }
            else
            {
                ESP_LOGW("HAN", "Malformed HDLC data");
            }

        }
    }
}


int16_t HANDecoder::getCosemLength(const uint8_t *buffer, const int16_t bufferSize)
{
    // Find the length of the OBIS record at buffer[0]
    uint16_t length = 0;
    if (bufferSize < 2)
    {
        ESP_LOGW("COSEM", "Invalid COSEM coding");
        return -1;
    }

    if (buffer[0] == 0x02)
    {
        // A struct
        uint8_t nElements = buffer[1];
        length = 2;

        // Sum the length of all elements
        for (int element = 0; element < nElements; element++)
        {
            length += getCosemLength(&buffer[length], bufferSize-length);
        }
    }
    else if (buffer[0] == 0x09)
    {
        // octet string
        uint8_t strLen = buffer[1];
        length += 2 + strLen;
    }
    else if (buffer[0] == 0x06)
    {
        // Unsigned 32
        length += 5;
    }
    else if (buffer[0] == 0x10)
    {
        // Int16
        length += 3;
    }
    else if (buffer[0] == 0x12)
    {
        // Unsigned int 16
        length += 3;
    }
    else if (buffer[0] == 0x0f)
    {
        // int 8
        length += 2;
    }
    else if (buffer[0] == 0x16)
    {
        // (0x16 = 22) enum uint8
        length += 2;
    }
    else
    {
        ESP_LOGW("COSEM", "Unknown COSEM type %d", buffer[0]);
        return -1;
    }
    return length;
}

uint16_t HANDecoder::decodeCosemFrame(const uint8_t *buffer, const uint16_t bufferSize)
{
    /// This method simply iterates over all COSEM objects in the frame
    ///
    /// Any sensors that should be updated are invoked in this method
    /// First version does this statically, but it should be possible
    /// to implement a more dynamic decoding method. But that comes later.

    if (bufferSize < 4)
    {
        ESP_LOGW("COSEM", "COSEM frame is too small");
        return 0;
    }

    if (
        buffer[0] != 0x02 ||
        buffer[2] != 0x09 ||
        buffer[3] != 0x06
    )
    {
        // All OBIS messages starts with a struct where the first element is the OBIS idientifier (octet string)
        ESP_LOGW("COSEM", "Invalid COSEM format %02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3]);
        return 0;
    }
    else
    {
        int16_t pos = 0;
        // All OBIS messages starts with a struct, e.g. 0x02
        while (pos < bufferSize && buffer[pos] == 0x02)
        {
            ESP_LOGD("COSEM", "Decoding at buffer[%d] (%d bytes remaining)", pos, bufferSize-pos);

            // Get the length of the record
            int16_t recordLen = getCosemLength(&buffer[pos], bufferSize-pos);
            if (recordLen > (bufferSize-pos))
            {
                ESP_LOGW("COSEM", "Buffer overflow when decoding COSEM frame");
                return 0;
            }
            else if (recordLen > 10)
            {
                ESP_LOGD("COSEM", "Identified an COSEM record of length %d", recordLen);

                // Check with all the listeners if they're matching
                for(auto it = this->_listeners.begin(); it != this->_listeners.end(); it++)
                {
                    if ((*it)->matches(&buffer[pos]))
                    {
                        // The listener will decode
                        (*it)->decode(&buffer[pos]);
                        break;
                    }
                }
                
            }
            else if (recordLen < 0)
            {
                ESP_LOGW("COSEM", "COSEM fragment too small to contain OBIS identifier");
                return 0;
            }
            pos += recordLen;
        }
        // Decoded the entire block
        return pos;
    }
}

bool HANDecoder::decodeDlms(const uint8_t *buffer, const uint16_t bufferSize)
{
    /*

    decodeDLMS

    https://github.com/roarfred/AmsToMqttBridge/blob/master/Documentation/Excerpt_GB8.pdf

    0:  0f          <- APDU Tag
    1:  40000000    <- invoke ID (ignored) "Long-Invoke-Id-And-Priority
    5:  00          <- Extra header size (byte)
    6:  011b        <- COSEM PDU size
    8:  0202 0906 0000010000ff 090c 07e30c1001073b28ff8000ff
    xx: 0203 0906 0100010700ff 06 00000462 0202 0f00 161b
*/
    // This is only called after a valid HDLC frame is received

    if (bufferSize > 5)
    {
        ESP_LOGD("han", "DLMS frame: %02x %02x %02x %02x %02x %02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    }

    if (bufferSize < 9)
    {
        ESP_LOGW("han", "Frame is too small");
        return false;
    }
    else if ( buffer[0] != 0x0f )
    {
        ESP_LOGW("han", "APDU should start with tag 0x0f, not 0x%02X", buffer[0]);
        return false;
    }

    // Compute the size of the header.
    uint8_t extraHeaderSize = buffer[5];    //< Extra header size is found in byte 5
    uint16_t pos = 6 + extraHeaderSize;
    uint16_t dataSize = (buffer[pos] << 8 ) + buffer[pos+1];
    pos += 2;       // Skip the dataSize


    // Iterate through the data in the package
    // All data is a struct beginning with an octet string that identifies the data

    uint16_t cosemSize = decodeCosemFrame(&buffer[pos], bufferSize-pos);
    if (cosemSize > 0)
    {
        // OBIS data was decoded successfully
        pos += cosemSize;
        // Only checksum should remain (end flag is removed by HDLC frame)

        if (bufferSize == pos)
        {
            ESP_LOGD("HAN", "Entire message is decoded");
        }
        else
        {
            ESP_LOGW("Han", "Stray bytes in HAN message: bufferSize=%d, pos=%d", bufferSize, pos);
            if (bufferSize - pos < 16)
            {
                while (pos < bufferSize)
                {
                    ESP_LOGD("HAN", "buffer[%d] = 0x%0x", pos, buffer[pos]);
                    pos++;
                }
            }
        }
        return true;
    }
    else
    {
        // OBIS decoding failed.
        return false;
    }

}

}   // namespace han
}   // namespace esphome
