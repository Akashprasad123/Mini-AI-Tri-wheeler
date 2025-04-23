#include "vl53l0x.h"
#include "gat_svc.h"
#include "common.h"
#include <esp_log.h>

#define TAG "VL53L0X"

/* Global Variable */
sensor_data_t *sensor_data = NULL;
esp_timer_handle_t tof_timer = NULL;
TaskHandle_t sensor_task_handle = NULL;

/* Private functions Declaration */
static VL53L0X_Error setup_vl53l0x(VL53L0X_Dev_t *sensor, uint8_t address);


/* Private Functions */

// Function to initialize VL53L0X for reading
static VL53L0X_Error setup_vl53l0x(VL53L0X_Dev_t *sensor, uint8_t address) {
    VL53L0X_Error status;
    VL53L0X_DeviceInfo_t devinfo;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    
    // Assign I2C address
    sensor->i2c_address = 0x29;
    sensor->i2c_port_num = I2C_MASTER_NUM;

    // Initialize Sensor
    status = VL53L0X_DataInit(sensor);
    if (status != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to datainit");
        return status;
    }

    address &= 0x7F;
    status = VL53L0X_SetDeviceAddress(sensor, address * 2);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (status != VL53L0X_ERROR_NONE){
        ESP_LOGE(TAG, "Failed to set device address");
        return status;
    }else{
        sensor->i2c_address = address;
    }

    if((status = VL53L0X_GetDeviceInfo(sensor, &devinfo)) == VL53L0X_ERROR_NONE){
        vTaskDelay(pdMS_TO_TICKS(100));
        status = VL53L0X_StaticInit(sensor);
        if (status != VL53L0X_ERROR_NONE) {
            ESP_LOGE(TAG, "Failed to staticinit");
            return status;
        }
        status = VL53L0X_PerformRefSpadManagement(sensor, &refSpadCount, &isApertureSpads);
        if (status != VL53L0X_ERROR_NONE) {
            ESP_LOGE(TAG, "SPAD Management failed with error code: %d\n", status);
            return status;
        }

    }

    // Start Measurement
    return VL53L0X_StartMeasurement(sensor);
}

/* Public Functions */

/* Function to initialize the VL53L0X sensor */
void vl53l0x_init(sensor_data_t *sensor_data){
    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                       0, 
                       0, 0);

    vTaskDelay(pdMS_TO_TICKS(100));

    //init gpio for XSHUT
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << XSHUT_PIN_1) | (1ULL << XSHUT_PIN_2) | (1ULL << XSHUT_PIN_3),
        .mode = GPIO_MODE_OUTPUT,  // Open-drain mode
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Enable internal pull-up
    };
    gpio_config(&io_conf);

    //disable all sensors
    gpio_set_level(XSHUT_PIN_1, 0);
    gpio_set_level(XSHUT_PIN_2, 0);
    gpio_set_level(XSHUT_PIN_3, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    //initialize all sensors
    // Setup Sensors
    gpio_set_level(XSHUT_PIN_1, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (setup_vl53l0x(&sensor_data[0].sensor, SENSOR_1_ADDR) != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to initialize Sensor 1");
        return;
    }

    gpio_set_level(XSHUT_PIN_2, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (setup_vl53l0x(&sensor_data[1].sensor, SENSOR_2_ADDR) != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to initialize Sensor 2");
        return;
    }

    gpio_set_level(XSHUT_PIN_3, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (setup_vl53l0x(&sensor_data[2].sensor, SENSOR_3_ADDR) != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to initialize Sensor 3");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Small delay to avoid overloading I2C

    ESP_LOGI(TAG, "All sensors initialized successfully!");

}

// Function to read distance from VL53L0X
int get_distance(VL53L0X_Dev_t *sensor) {
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_Error status;

    status = VL53L0X_PerformSingleRangingMeasurement(sensor, &measurement);
    
    if (status != VL53L0X_ERROR_NONE) {
        ESP_LOGE(TAG, "Error in ranging: %d", status);
        return -1;
    }

    // Check if the measurement is valid
    if (measurement.RangeStatus != 0 || measurement.RangeStatus == 2 || measurement.RangeStatus == 4) { 
        // ESP_LOGW(TAG, "Invalid measurement: Status = %d", measurement.RangeStatus);
        return 1300; // Return max distance if invalid
    }

    return measurement.RangeMilliMeter;
}

void sensor_read_task(void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for notification from timer
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to avoid overloading I2C
        for (int i = 0; i < NUM_OF_SENSORS; i++) {
            int distance = get_distance(&sensor_data[i].sensor);
            if (distance >= 0) {
                sensor_data[i].distance = distance;
            } else {
                sensor_data[i].distance = 1300;  // Default if read fails
                ESP_LOGW(TAG, "Sensor %d Read Error!", i);
            }
        }

        // ESP_LOGI("VL53L0X", "Sensor 0: %d mm | Sensor 1: %d mm | Sensor 2: %d mm", 
        //          sensor_data[0].distance, sensor_data[1].distance, sensor_data[2].distance);
        send_sensor_value_indication();  // BLE Indication from gat_svc.c
    }
}

// ESP-Timer callback function
void tof_sensor_callback(void *arg) {
    if (sensor_task_handle) {
        xTaskNotifyGive(sensor_task_handle);  // Notify the task to read sensors
    }
}

void get_sensor_data(int* sensor_values) {
    for (int i = 0; i < NUM_OF_SENSORS; i++) {
        sensor_values[i] = sensor_data[i].distance;
    }
}
