#ifndef VL53L0X_H
#define VL53L0X_H

/* Includes */
#include "driver/i2c_master.h"
#include <esp_log.h>
#include "vl53l0x_api.h"
#include "common.h"

/* Defines */
#define NUM_OF_SENSORS       3          // Number of sensors connected
#define VL53L0X_QUEUE_SIZE   10         // Queue size for sensor data

#define I2C_MASTER_NUM       I2C_NUM_0  // I2C Port
#define I2C_MASTER_SDA_IO    5 //22         // SDA Pin
#define I2C_MASTER_SCL_IO    6 //23         // SCL Pin
#define I2C_MASTER_FREQ_HZ   100000     // I2C Clock Speed

#define SENSOR_1_ADDR        0x30       // I2C Address for Sensor 1
#define SENSOR_2_ADDR        0x31       // I2C Address for Sensor 2
#define SENSOR_3_ADDR        0x32       // I2C Address for Sensor 2

#define XSHUT_PIN_1            44 //18         // GPIO Pin for XSHUT
#define XSHUT_PIN_2            7 //19         // GPIO Pin for XSHUT
#define XSHUT_PIN_3            8 //20         // GPIO Pin for XSHUT

#define SENSOR_TASK_STACK_SIZE 4096
#define SENSOR_TASK_PRIORITY   5


/* Typedefs */
typedef struct {
    VL53L0X_Dev_t sensor;
    int distance;
    int sensor_index;
} sensor_data_t;

extern sensor_data_t *sensor_data;
extern esp_timer_handle_t tof_timer;
extern TaskHandle_t sensor_task_handle;


/* Public function declarations */
void vl53l0x_init(sensor_data_t *sensor_data);
int get_distance(VL53L0X_Dev_t *sensor);
void tof_sensor_callback(void *arg);
void get_sensor_data(int* sensor_values);
void sensor_read_task(void *arg);

// extern SemaphoreHandle_t sensor_lock;

#endif // VL53L0X_H