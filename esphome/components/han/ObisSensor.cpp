#include "ObisSensor.h"
#include <ctime>
#include <string>

namespace esphome {
namespace han {

static const uint8_t OBIS_UINT32_T = 0x06;
static const uint8_t OBIS_OCTET_STRING_T = 0x09;
static const uint8_t OBIS_INT16_T  = 0x10;
static const uint8_t OBIS_UINT16_T = 0x12;
static const uint8_t OBIS_INT8_T   = 0x0f;
static const uint8_t OBIS_UINT8_T  = 0x16;

bool ObisSensor::decode(const uint8_t* data)
{
    // payload starts at byte 10
    const uint8_t* buffer = &data[10];

    if (buffer[0] == OBIS_UINT32_T)
    {
        // Unsigned int 32
        uint32_t value = 0;
        for(int k=0; k<4; k++)
        {
            value = (value << 8) | buffer[k+1];
        }
        publish_state(value);
        return true;
    }
    else if (buffer[0] == OBIS_INT16_T)
    {
        // Int16
        int16_t value = 0;
        for(int k=0; k<2; k++)
        {
            value = (value << 8) | buffer[k+1];
        }
        publish_state(value);
        return true;
    }
    else if (buffer[0] == OBIS_UINT16_T)
    {
        // Unsigned int 16
        uint16_t value = 0;
        for(int k=0; k<2; k++)
        {
            value = (value << 8) | buffer[k+1];
        }
        publish_state(value);
        return true;
    }
    else if (buffer[0] == OBIS_INT8_T)
    {
        // int 8
        int8_t value = buffer[1];
        publish_state(value);
        return true;
    }
    else if (buffer[0] == OBIS_UINT8_T)
    {
        // (0x16 = 22) enum uint8
        uint8_t value = buffer[1];
        publish_state(value);
        return true;
    }
    else if (buffer[0] == OBIS_OCTET_STRING_T && buffer[1] == 0x0c)
    {
        // Special case for Time
        return decodeTime(buffer);
    }
    else
    {
        ESP_LOGW("OBIS", "Unknown DLMS type %d", buffer[0]);
        return false;
    }
}

bool ObisSensor::decodeTime(const uint8_t* buffer)
{
    // payload starts at byte 10
    const uint8_t* timeBuffer = &buffer[2];

    if (buffer[0] != 0x09 &&
        buffer[1] != 0x0c
    )
    {
        ESP_LOGW("TimeObis", "Malformed Timestamp OBIS message");
        return false;
    }
    else
    {
        ESP_LOGD("TimeObis", "Received timestamp");
        std::tm t;
        t.tm_sec = 0;
        t.tm_year = ((timeBuffer[0] << 8) | timeBuffer[1]) - 1900;
        t.tm_mon = timeBuffer[2] - 1;
        t.tm_mday = timeBuffer[3];
        //t.tm_wday = timeBuffer[4];    //< Not used by mktime
        t.tm_hour = timeBuffer[5];
        t.tm_min = timeBuffer[6];
        t.tm_sec = timeBuffer[7];
        
        // Daylight saving: -1 = Unknown, >0 means DST is active (summer), 0 means not
        if (timeBuffer[8] == 0)   
        {         
            // Not DST
            t.tm_isdst = 0;
        }
        else if (timeBuffer[8] & 0x80 )     
        {
            // Unknown
            t.tm_isdst = 0;
        }
        else
        {
            t.tm_isdst = 1;
        }

        char timeStr[32];
        std::strftime(timeStr, 32, "%Y-%m-%d %H:%M:%S", &t);
        ESP_LOGI("TimeObis", "Time is: %s", timeStr);
        // Convert timestamp to POSIX time
        auto ts = std::mktime(&t);

        if (t.tm_isdst)
        {
            ESP_LOGD("TimeObis", "DST active");
            // Adjust for DST
            ts -= 3600;
        }
        else
        {
            ESP_LOGD("TimeObis", "DST inactive");
        }

        publish_state(ts);
        return true;
    }
}

}   // namespace han
}   // namespace esphome