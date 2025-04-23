#ifndef MOTOR_H
#define MOTOR_H

/* Includes */
#include "driver/ledc.h"
#include "common.h"

#define NUM_MOTORS          2
#define MIN_SPEED_PERCENT   50
#define MAX_DUTY            ((1 << LEDC_TIMER_10_BIT) - 1)
#define MIN_DUTY            (MAX_DUTY * 0.2)

#define MOTOR_TASK_STACK_SIZE  4096
#define MOTOR_TASK_PRIORITY    5

#define MOTOR_FWD_CHANNEL0      LEDC_CHANNEL_0
#define MOTOR_FWD_CHANNEL1      LEDC_CHANNEL_1
#define MOTOR_BKWD_CHANNEL0     LEDC_CHANNEL_2
#define MOTOR_BKWD_CHANNEL1     LEDC_CHANNEL_3

extern TaskHandle_t motor_task_handle;

/* Public function declarations */
void config_pwn_timer(void);
void config_motor_pwm(int gpio, ledc_channel_t channel);
void set_motor_speed(int motor_index, int target_speed, bool direction);
void stop_motors(void);
void set_motor_state_task(void *arg);
void get_motor_control_value(int* data);
void set_motor_control_value(int* data);


#endif // MOTOR_H