#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sensor_tasks.h"
#include "i2c_config.h"
#include "esp_check.h"
#include "co2_sensor.h"
#include "voc_sensor.h"

#define TEMP_SENS_ADDR     0x44
#define TEMP_MEASURE_CMD   0xFD

#define CO2_SENS_ADDR_A    0x62     //0x29
#define CO2_SENS_ADDR_B    0x2A

#define VOC_SENS_ADDR      0x59

#define CRC8_INIT          0xFF
#define CRC8_POLYNOMIAL    0x31
#define CRC8_LEN           1

QueueHandle_t co2_data_queue = NULL;
SemaphoreHandle_t co2_mutex = NULL;

/* CRC check for PCB CO2 sensor, not currently used with bredboard sensor
esp_err_t crc_check(const uint8_t* data, uint16_t count, uint8_t checksum)
{
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    // calculates 8-Bit checksum with given polynomial
    for (current_byte = 0; current_byte < count; ++current_byte) 
    {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) 
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    if(crc != checksum)
    {
        ESP_LOGE("CRC", "Error with CRC");
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI("CRC", "CRC Successful");
        return ESP_OK;
    }
}

*/
#ifdef TEMP_SENSOR_CONNECTED
void temp_humidity_task(void *parameter)
{
    while(1)
    {
        esp_err_t err = ESP_FAIL;
        uint8_t data[6] = {0};      //2 bytes for temp reading + 1 CRC. Then two bytes for humidity reading + 1 CRC
        uint8_t temp_cmd = TEMP_MEASURE_CMD;

        //if these commands work, don't need commented section
        err = i2c_master_write_to_device(I2C_PORT, TEMP_SENS_ADDR, &temp_cmd, 1, I2C_TIMEOUT);
        if(err != ESP_OK)
        {
            ESP_LOGE("TEMP", "Could not write command to temp sensor");
            continue;
        }

        //let the processor do other things while we wait for the sensor to finish its measuremetn ~ 10ms
        vTaskDelay(pdMS_TO_TICKS(10));

        err = i2c_master_read_from_device(I2C_PORT, TEMP_SENS_ADDR, data, sizeof(data), I2C_TIMEOUT);
        if(err != ESP_OK)
        {
            ESP_LOGE("TEMP", "Could not read from temp sensor");
            continue;
        }
        //check temperature CRC
        err = crc_check(&data[0], 2, data[2]);
        if(err != ESP_OK)
        {
            ESP_LOGE("CRC", "CRC Temperature Check Failed");
            continue;   //skip any further computation if CRC check fails and restart loop
        }

        //check humidity crc
        err = crc_check(&data[3], 2, data[5]);
        if(err != ESP_OK)
        {
            ESP_LOGE("CRC", "CRC Humidity Check Failed");
            continue;   //skip any further computation if CRC check fails and restart loop
        }

//Will need to setup a switch of if statemment depending on what mode the user has device in for F/C
        // for celsius
        //uint16_t raw_temp = (data[0] << 8) | data[1];
        //float temperature = -45 + 175 * (raw_temp / 65535.0);     // Temperature in Celsius

        //for farenheit
        uint16_t raw_temp = (data[0] << 8) | data[1];
        float temperature = -49 + 315 * (raw_temp / 65535.0);     //Temerature in Farenheit

        uint16_t raw_humidity = (data[3] << 8) | data[4];
        float humidity = -6 + 125 * (raw_humidity / 65535.0);     // Relative humidity in %

        ESP_LOGI("TEMP/HUMID", "Measured Temperatue: %f\n Measured Humidity: %f", temperature, humidity);
    }
}

#endif

#ifdef VOC_SENSOR_CONNECTED
void voc_task(void *parameter)
{
    while(1)
    {
        uint8_t command[6] = {0x26, 0x0F, 0x00, 0x00, 0x00, 0x00};  //sensor takes a 6 byte command
        uint8_t sensor_a_data[3] = {0};
        uint8_t sensor_b_data[3] = {0};
        esp_err_t err = ESP_FAIL;

        for(uint8_t j = 0; j < 2; j++)  //j = 0 sends the write command for sensor to take measurement, j = 1 will read measured value
        {
            for(uint8_t i = 0; i < 2; i++)
            {
                if(j == 0)  //send command for sensor to take measurement
                {
                    //select multiplexer channel, then write the command to the sensor to get a reading
                    voc_select_channel(i);
                    err = i2c_master_write_to_device(I2C_PORT, VOC_SENS_ADDR, command, sizeof(command), I2C_TIMEOUT);
                    if(err != ESP_OK)
                    {
                        ESP_LOGE("VOC", "Failed to write command to sensor");
                    }
                }
                else if(j == 1 && i == 0) // read maeasured data from channel 0 and store in sensor a
                {
                    voc_select_channel(i);
                    err = i2c_master_read_from_device(I2C_PORT, VOC_SENS_ADDR, sensor_a_data, sizeof(sensor_a_data), I2C_TIMEOUT);
                    if(err != ESP_OK)
                    {
                        ESP_LOGE("VOC", "Failed to read measurement from VOC sensor A");
                    }
                }
                else if(j == 1 && i == 1) // read maeasured data from channel 1 and store in sensor b
                {
                    voc_select_channel(i);
                    err = i2c_master_read_from_device(I2C_PORT, VOC_SENS_ADDR, sensor_b_data, sizeof(sensor_b_data), I2C_TIMEOUT); 
                    if(err != ESP_OK)
                    {
                        ESP_LOGE("VOC", "Failed to read measurement from VOC sensor B");
                    }
                }
            }
            //after both sensor read commands have been sent, allow time for sensors to process
            if( j == 0) //only delay after we send command to start measurement
            {
                vTaskDelay(pdMS_TO_TICKS(30));
            }
        }

        //crc check sensor a data
        err = crc_check(&sensor_a_data[0], 2, sensor_a_data[2]);
        if(err != ESP_OK)
        {
            ESP_LOGE("VOC CRC", "CRC check on VOC measurement unsuccessful");
            continue;
        }

        //crc check sensor b data
        err = crc_check(&sensor_b_data[0], 2, sensor_b_data[2]);
        if(err != ESP_OK)
        {
            ESP_LOGE("VOC CRC", "CRC check on VOC measurement unsuccessful");
            continue;
        }

        //convert data to readable format
    }
}
#endif

/**
 * 
 * @brief This task handles communication between the MCU and the CO2 sensor.
 *        It also handles all logic that needs to be performed on the data read
 */
void co2_task(void *parameter)
{
    // 32 start CO2 sensor command split byte by byte so that it can be used in the transmit function
    uint8_t co2_start_cmd[2] =    {0x21, 0xb1};                          //{0x36, 0x15, 0x00, 0x11};  //the two commands split into byte by byte  
    
    // create co2 queue and mutex for data transfer with web UI task
    co2_data_queue = xQueueCreate(10, sizeof(uint16_t));
    co2_mutex = xSemaphoreCreateMutex();
    if(co2_data_queue == NULL)
    {
        ESP_LOGE("CO2", "Error creating co2 data queue");
    }
    if(co2_mutex == NULL)
    {
        ESP_LOGE("CO2", "Error creating co2 mutex");
    }

    // give time for I2C to initialize before trying to read from sensor on startup
    vTaskDelay(pdMS_TO_TICKS(30));

    while(1) // continuous task loop
    {
        esp_err_t err = ESP_FAIL;
        uint16_t sensor_a_raw_concentration = 0;
        uint16_t sensor_b_raw_concentration = 0;
        float co2_measurement_a = 0;
        float co2_measurement_b = 0;

        for(uint8_t j = 0; j < 2; j++)  // j = 0 will write the command to sensor to begin measurement. j = 1 will read measurement data from sensors
        {
            if(j == 0)  // send command to first sensor to take measurement
            {
                err = i2c_master_transmit(i2c_co2_device_handle, co2_start_cmd, sizeof(co2_start_cmd), pdMS_TO_TICKS(100));
                if(err != ESP_OK)
                {
                    ESP_LOGE("CO2 TRANSMIT", "Error writing measure command to sensor with error: 0x%03x", err);
                }

                // after both sensors receive initial command, give time to take measurement
                vTaskDelay(pdMS_TO_TICKS(5000));
            }    
            else if(j == 1)  // read measurement from sensor A
            {
                if(co2_read_data(&sensor_a_raw_concentration) == ESP_OK)
                {
                    ESP_LOGI("CO2 Reading", "PPM: %d", sensor_a_raw_concentration);
                    //co2_measurement_a = ((sensor_a_raw_concentration - 16384.0) / 32768.0) * 100;
                }
            }

            if(j == 1) // make sure both sensor measured similar values, average out, and add to queue
            {
                if(did_both_co2_sensors_read_valid(co2_measurement_a, co2_measurement_b) == true)
                {
                    float avg_co2 = (co2_measurement_a + co2_measurement_b) / 2;
                    //err = xQueueSend(co2_readings_queue, avg_co2, pdMS_TO_TICKS(10));
                    //if(err != ESP_OK)
                    //{
                    //  ESP_LOGW("CO2", "Failed to add data to queue");
                   // }
                }
                else  //somehow figure out which one was accurate and which one to use
                {

                }

                //convert_co2_data_to_readable(&sensor_a_raw_concentration);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        //convert data to readable format
    }
}

