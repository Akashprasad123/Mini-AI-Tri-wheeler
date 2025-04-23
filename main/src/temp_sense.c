#include "temp_sense.h"
#include "common.h"

#define TAG "TEMP_SENSOR"

/* Public Variables */
TaskHandle_t temp_read_task_handle = NULL;


/* Private Variable */
static float temperature = 0.0f;
static temperature_sensor_handle_t temp_sensor_handle = NULL;

void init_temp_sensor(temperature_sensor_config_t temp_sensor) {
    // Initialize temperature sensor
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor, &temp_sensor_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor_handle));
}

void read_temp_task(void *arg) {
    while (1) {
        // Read temperature in Celsius
        ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor_handle, &temperature));
        ESP_LOGI(TAG, "Temperature: %.2f °C", temperature);
        vTaskDelay(pdMS_TO_TICKS(TEMP_SENSOR_READ_INTERVAL)); // Delay for 1 second
    }
}