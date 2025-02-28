#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "general_sensors.h"
#include "i2c_config.h"
#include "esp_check.h"
#include "co2_sensor.h"
#include "voc_sensor.h"

#define CRC_INIT          0xFF
#define CRC_POLYNOMIAL    0x31

/***************
 * @brief CRC function for all sensors
 * @param data the data read from the sensor
 * @param count amount of bytes
 ***************/
uint8_t crc_check(const uint8_t* data, uint16_t count) 
{
    uint16_t current_byte;
    uint8_t crc = CRC_INIT;
    uint8_t crc_bit;
    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) 
    {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) 
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}
