#include "motor.h"
#include "esp_log.h"
#include <stdio.h>

#define TAG "MOTOR"

TaskHandle_t motor_task_handle = NULL;

/* Private variable */
static int motor_ctrl_value[2] = {0, 90}; // X-axis and Y-axis values

void config_pwn_timer(){
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
}

void config_motor_pwm(int gpio, ledc_channel_t channel){

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = channel,
        .gpio_num = gpio,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

}

void stop_motors(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL0, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL1, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL0, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL1, 0);

    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL1);
}

void set_motor_state_task(void *arg) {

    while(1){
        int left_duty = 0, right_duty = 0;
        bool direction = (motor_ctrl_value[1]> 0); // True: forward, False: backward
    
        int abs_y = abs(motor_ctrl_value[1]);  // Get absolute speed value
    
        // Smooth speed curve for better control
        int scaled_duty = (abs_y < 50) ? (MAX_DUTY * (abs_y / 50.0) / 2) : (MAX_DUTY * abs_y / 100);
    
        // Smooth steering control (reduce sudden stops)
        float turn_factor = 1.0 - (abs(motor_ctrl_value[0]) / 200.0); // Adjust how much speed is reduced per wheel
        if (motor_ctrl_value[0] < 0) {  
            left_duty = scaled_duty * turn_factor;
            right_duty = scaled_duty;
        } else {
            left_duty = scaled_duty;
            right_duty = scaled_duty * turn_factor;
        }
    
    
        // Stop opposite direction first
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL1, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL1, 0);
    
        // Apply speed and direction
        if (direction) { 
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL0, right_duty);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL1, left_duty);
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL0, left_duty);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL1, right_duty);
        }
    
        // Update duty cycle
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_FWD_CHANNEL1);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_BKWD_CHANNEL1);

        // printf("Motor Control : [%d, %d]\n", motor_ctrl_value[0], motor_ctrl_value[1]);
        vTaskDelay(pdMS_TO_TICKS(200)); // Adjust delay as needed
    }
   
}

void get_motor_control_value(int* data) {
    data[0] = motor_ctrl_value[0]; // X-axis value
    data[1] = motor_ctrl_value[1]; // Y-axis value
}

void set_motor_control_value(int* data) {
    motor_ctrl_value[0] = data[0]; // X-axis value
    motor_ctrl_value[1] = data[1]; // Y-axis value
}
