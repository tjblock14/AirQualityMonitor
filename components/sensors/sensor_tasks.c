#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sensor_tasks.h"
#include "pin_config.h"
#include "esp_check.h"
#include "co2_sensor.h"
#include "voc_sensor.h"

#define TEMP_SENS_ADDR     0x44
#define TEMP_MEASURE_CMD   0xFD

#define CO2_SENS_ADDR_A    0x29
#define CO2_SENS_ADDR_B    0x2A

#define VOC_SENS_ADDR      0x59

#define CRC8_INIT          0xFF
#define CRC8_POLYNOMIAL    0x31
#define CRC8_LEN           1

esp_err_t crc_check(const uint8_t* data, uint16_t count, uint8_t checksum)
{
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
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

        /** 
        //start I2C communication
        ESP_ERROR_CHECK(i2c_master_start(cmd));

        //tell the sensor that we want to read data from it by writing its address and command
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (TEMP_SENS_ADDR << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK));
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, TEMP_MEASURE_CMD, I2C_MASTER_ACK));
        ESP_ERROR_CHECK(i2c_master_stop(cmd));

        //let the processor do other things while we wait for the sensor to finish its measuremetn ~ 10ms
        ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT)));
        ESP_ERROR_CHECK(i2c_cmd_link_delete(cmd));
        vTaskDelay(10000);

        //setup the cmd for communication again so we can read the measured value
        cmd = i2c_cmd_link_create();
        ESP_ERROR_CHECK(i2c_master_start(cmd));
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (TEMP_SENS_ADDR << 1) | I2C_MASTER_READ, I2C_MASTER_ACK));
        ESP_ERROR_CHECK(i2c_master_read(cmd, data, sizeof(data) - 1, I2C_MATER_ACK));
        ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data[5]), I2C_MASTER_NACK);
        ESP_ERROR_CHECK(i2c_master_stop(cmd));
        ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT)));
        ESP_ERROR_CHECK(i2c_cmd_link_delete(cmd));

        */
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


/**
 * 
 * @brief task to write command to co2 sensor to initialize a reading. Then, give sensor enough time
 *        to measure and then get the measured value and convert to readable format before adding to queue
 */
void co2_task(void *parameter)
{
    while(1)
    {
        esp_err_t err = ESP_FAIL;

        uint8_t co2_cmd[4] = {0x36, 0x15, 0x00, 0x11};  //the two commands split into bit by bit
        uint8_t read_cmd[2] = {0x16, 0x39};
        uint8_t sensor_address = 0x00;

        uint16_t sensor_a_raw_concentration = 0;
        uint16_t sensor_b_raw_concentration = 0;
        float co2_measurement_a = 0;
        float co2_measurement_b = 0;

        for(uint8_t j = 0; j < 2; j++)  //j = 0 will write the measurement command to both sensors. j = 1 will read measurements from sensors
        {
            for(uint8_t i = 0; i < 2; i++)  //i = 1 will handle sensor A, i = 2 will handle sensor B
            {
                switch (i)  //interact with sensor A when i == 0 and sensor B when i == 1
                {
                    case 0:
                        sensor_address = CO2_SENS_ADDR_A;
                        break;
                    case 1: 
                        sensor_address = CO2_SENS_ADDR_B;
                        break;
                    default:
                        break;
                }

                if(j == 0)
                {
                    err = i2c_master_write_to_device(I2C_PORT, sensor_address, co2_cmd, sizeof(co2_cmd), I2C_TIMEOUT);
                    if(err != ESP_OK)
                    {
                        ESP_LOGE("CO2", "Failed to write command to sensor");
                    }
                }    
                else if(j == 1 && i == 0)  //read measurement from sensor A
                {
                    if(co2_read_data(sensor_address, &sensor_a_raw_concentration) == ESP_OK)
                    {
                        co2_measurement_a = ((sensor_a_raw_concentration - 16384.0) / 32768.0) * 100;
                    }
                }
                else if(j == 1 && i ==1) // read measurement from sensor B
                {
                    if(co2_read_data(sensor_address, &sensor_b_raw_concentration) == ESP_OK)
                    {
                        co2_measurement_b = ((sensor_b_raw_concentration - 16384.0) / 32768.0) * 100;
                    }
                }
            }
            if(j == 0)//give sensors enough time to complete measurement if first iteration
            {
                vTaskDelay(pdMS_TO_TICKS(110));
            }

            if(j == 1) // make sure both sensor measured similar values, average out, and add to queue
            {
                if(did_both_co2_sensors_read_valid(co2_measurement_a, co2_measurement_b) == true)
                {
                    float avg_co2 = (co2_measurement_a + co2_measurement_b) / 2;
                    //err = xQueueSend(co2_readings_queue, avg_co2, pdMS_TO_TICKS(10));
                    if(err != ESP_OK)
                    {
                        ESP_LOGW("CO2", "Failed to add data to queue");
                    }
                }
                else  //somehow figure out which one was accurate and which one to use
                {

                }
            }
        }

        //convert data to readable format
    }
}

