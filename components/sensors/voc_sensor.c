#include "stdint.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_config.h"
#include "general_sensors.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define VOC_SENS_ADDR      0x59
#define VOC_MUX_ADDR       0x70

/*******************************
 * @brief This function determines which port of the multiplexer to use.
 * @param channel is the channel of the multiplexer, either 0 or 1. This determines which sensor to communicate with
 ********************************/
void voc_select_channel(uint8_t channel)
{
    esp_err_t err = ESP_FAIL;

    uint8_t channel_select = 0x04 + channel;  //0x04 corresponds to channel 0, 0x05 corresponds to channel 1

    //write to the channel of the multiplexer we want to communicate with
    err = i2c_master_write_to_device(I2C_PORT, VOC_MUX_ADDR, &channel_select, 1, I2C_TIMEOUT);
    if(err != ESP_OK)
    {
        ESP_LOGE("VOC MUX", "Could not select mux channel");
    }
}

void voc_task(void *parameter)
{
    while(1)
    {
        voc_data_queue = xQueueCreate(10, sizeof(uint16_t));
        uint8_t command[6] = {0x26, 0x0F, 0x80, 0x00, 0xA2, 0x00};  //sensor takes a 6 byte command
        uint8_t sensor_a_data[3] = {0};
        uint8_t sensor_b_data[3] = {0};
        esp_err_t err = ESP_FAIL;

        for(uint8_t j = 0; j < 2; j++)  //j = 0 measures sensor a, j =  will read measured value
        {

        }

        // Perform CRC check on sensor data
        if(crc_check(sensor_a_data, 2) == ESP_OK)
        {

        }

        if(crc_check(sensor_b_data, 2) == ESP_OK)
        {

        }
        

        //convert data to readable format
    }
}