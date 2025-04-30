/*********
 * Thsi file initializes pins on the esp32-S2_Solo-2
 * 
 * 
 *********/

 #include "driver/i2c_master.h"
 #include "i2c_config.h"
 #include "esp_err.h"
 #include "esp_log.h" 
 #include "driver/ledc.h"
 #include "driver/gpio.h"
 
 #define LEDC_OUTPUT_PIN 12 
 #define LEDC_CHANNEL LEDC_CHANNEL_0 // Use one of the 8 channels
 #define LEDC_TIMER LEDC_TIMER_0     // Use one of the 4 timers
 #define LEDC_MODE LEDC_HIGH_SPEED_MODE // LEDC has high and low speed modes
 #define LEDC_FREQUENCY 490      
   
 
 static const char *TAG = "I2C";
 
 i2c_master_bus_handle_t i2c_bus_handle;
 
 // Setup for CO2 sensor as I2C device
 i2c_master_dev_handle_t i2c_co2_device_handle;
 i2c_device_config_t i2c_co2_device = {
     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
     .device_address = 0x62,
     .scl_speed_hz = I2C_MASTER_FREQ
 };
 
 i2c_master_dev_handle_t i2c_temp_device_handle;
 i2c_device_config_t i2c_temp_device = {
     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
     .device_address = 0x44,
     .scl_speed_hz = I2C_MASTER_FREQ
 };

 i2c_master_dev_handle_t i2c_voc_device_handle;
 i2c_device_config_t i2c_voc_device = {
     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
     .device_address = 0x58,
     .scl_speed_hz = I2C_MASTER_FREQ
 };
 
 // Setup for Display as I2C device
 i2c_master_dev_handle_t i2c_display_device_handle;
 i2c_device_config_t i2c_display_device = {
     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
     .device_address  = 0x72,
     .scl_speed_hz = I2C_MASTER_FREQ
 };
 
 //create devices for other sensors here
 
 // Configures the I2C interface for the ESP32 microcontroller
 // Also currentoly configures a PWM pin for the buzzer. This will later be its own init function
 void i2c_master_config()
 {
     i2c_master_bus_config_t i2c_conf = {
         .i2c_port = I2C_PORT,
         .scl_io_num = I2C_SCL_PIN,
         .sda_io_num = I2C_SDA_PIN,
         .clk_source = I2C_CLK_SRC_DEFAULT
     };
 
     esp_err_t err = i2c_new_master_bus(&i2c_conf, &i2c_bus_handle);
 
     if(err == ESP_OK)
     {
         ESP_LOGI(TAG, "I2C bus successfully created");
     }
     else
     {
         ESP_LOGE(TAG, "Error creating i2c bus: 0x%03X", err);
     }
 
     //add I2C devices to the created bus
     err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_co2_device, &i2c_co2_device_handle);
     if(err == ESP_OK)
     {
         ESP_LOGI(TAG, "CO2 Device added to I2C bus");
     }
     else
     {
         ESP_LOGE(TAG, "Error adding CO2 device to I2C bus: 0x%03X", err);
     }
     
     err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_display_device, &i2c_display_device_handle);
     if(err == ESP_OK)
     {
         ESP_LOGI(TAG, "Display Device added to I2C bus");
     }
     else
     {
         ESP_LOGE(TAG, "Error adding display device to I2C bus");
     }

     err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_temp_device, &i2c_temp_device_handle);
     if(err == ESP_OK)
     {
         ESP_LOGI(TAG, "Temp Device added to I2C bus");
     }
     else
     {
         ESP_LOGE(TAG, "Error adding temp device to I2C bus");
     }

     err = i2c_master_bus_add_device(i2c_bus_handle, &i2c_voc_device, &i2c_voc_device_handle);
     if(err == ESP_OK)
     {
         ESP_LOGI(TAG, "VOC Device added to I2C bus");
     }
     else
     {
         ESP_LOGE(TAG, "Error adding VOC device to I2C bus");
     }
 
     // Configure the LEDC timer for the PWM signal
     ledc_timer_config_t ledc_timer = {
         .timer_num = LEDC_TIMER,
         .duty_resolution = LEDC_TIMER_13_BIT, // 13-bit resolution
         .freq_hz = LEDC_FREQUENCY,            // Set frequency - Audrey previously tested with this f on Arduino
         .clk_cfg = LEDC_AUTO_CLK,
         .speed_mode = LEDC_LOW_SPEED_MODE
     };
     ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
 
     // Confiure the PWM signal
     ledc_channel_config_t pwm_conf = {
         .duty = 0,  // Begin at zero on startup
         .gpio_num = LEDC_OUTPUT_PIN,
         .channel = LEDC_CHANNEL_0,
         .speed_mode = LEDC_LOW_SPEED_MODE
     };
 
     ESP_ERROR_CHECK(ledc_channel_config(&pwm_conf));
 
     // Not sure if this is needed, looking into it
     ESP_ERROR_CHECK(ledc_fade_func_install(0));
 
 
 }