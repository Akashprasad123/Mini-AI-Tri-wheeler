#ifndef TEMP_SENSE_H
#define TEMP_SENSE_H

#include "driver/temperature_sensor.h"
#include "common.h"

#define TEMP_SENSOR_MIN_RANGE       -10
#define TEMP_SENSOR_MAX_RANGE       80
#define TEMP_SENSOR_CLK_DIV         6 // Clock divider (higher = slower sampling, lower power)
#define TEMP_TASK_STACK_SIZE      1024 // Stack size for temperature reading task
#define TEMP_TASK_PRIORITY        5 // Task priority for temperature reading task
#define TEMP_SENSOR_READ_INTERVAL 1000 // Interval for reading temperature in milliseconds

/* Global Variables */
extern TaskHandle_t temp_read_task_handle;

/* Public Function */
void init_temp_sensor(temperature_sensor_config_t temp_sensor);
void read_temp_task(void *arg);

#endif // TEMP_SENSE_H