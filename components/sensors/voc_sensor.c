#include "stdint.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_config.h"
#include "sensor_tasks.h"
#include "driver/i2c.h"

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
