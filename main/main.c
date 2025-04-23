/* Includes */
#include <stdlib.h>
#include "common.h"
#include "gap.h"
#include "led.h"
#include "motor.h"
#include "vl53l0x.h"
#include "gat_svc.h"
#include "temp_sense.h"
#include "dl_wrapper.h"
#include <esp_log.h>
#include "esp_psram.h"

#define TAG "MAIN"

/* Private variables */
static const float mean[5] = {167.17619687, 517.20985025, 182.52839401,  -5.76898122,  85.66035325};
static const float std[5] = {58.89175887, 241.57810185,  64.97819453,  52.67704403,  12.33903878};

/* Library function declarations */
void ble_store_config_init(void);

/* Private function declarations */
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);

/* Private functions */
/*
 *  Stack event callback functions
 *      - on_stack_reset is called when host resets BLE stack due to errors
 *      - on_stack_sync is called when host has synced with controller
 */
static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    adv_init();
}

static void nimble_host_config_init(void) {
    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
}

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}

static void collect_and_normalize_data(float *input_data) {
    int control_values[2] = {0, 0};  // [turn, speed]

    // Normalize sensor distances
    for (int i = 0; i < NUM_OF_SENSORS; i++) {
        input_data[i] = (sensor_data[i].distance - mean[i]) / std[i];
    }

    // Get previous control output (turn, speed)
    get_motor_control_value(control_values);

    input_data[NUM_OF_SENSORS] = (control_values[0] - mean[NUM_OF_SENSORS]) / std[NUM_OF_SENSORS];
}


static void denormalize_and_set_motor_value(float *output) {
    int current_control[2] = {0, 0};
    float updated_control[2];

    get_motor_control_value(current_control);

    // Convert current control values to normalized space
    // for (int i = 0; i < 2; i++) {
    updated_control[0] = (current_control[0] - mean[NUM_OF_SENSORS + 0]) / std[NUM_OF_SENSORS + 0];
    updated_control[0] += output[0];  // Add model-predicted Δchange
    updated_control[0] = updated_control[0] * std[NUM_OF_SENSORS + 0] + mean[NUM_OF_SENSORS + 0];
    // }

    // Clamp output values to valid range [-100, 100]
    // for (int i = 0; i < 2; i++) {
        if (updated_control[0] > 100) updated_control[0] = 100;
        if (updated_control[0] < -100) updated_control[0] = -100;
        current_control[0] = (int)updated_control[0];
    // }
    printf("Motor Control : [%d, %d]", current_control[0], current_control[1]);
    set_motor_control_value(current_control);  // Set new [turn, speed] control
}


static void model_run_task(void *arg){
    float normalized_input[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float *output = NULL;
    while(1){
        collect_and_normalize_data(normalized_input);
        wrapper_run_model_inference(normalized_input, &output);
        ESP_LOGI(TAG, "Model Output : [%f, %f]", output[0], output[1]);
        denormalize_and_set_motor_value(output);
        vTaskDelay(pdMS_TO_TICKS(50));
        wrapper_clean();
    }
}



void app_main(void) {
    /* Local variables */
    int rc = 0;
    esp_err_t ret = ESP_OK;

    /* Initialize GPIO for builtin led*/
    gpio_init();

    /* Initialize Motor PWM */
    config_pwn_timer();
    config_motor_pwm(43, MOTOR_FWD_CHANNEL0);
    config_motor_pwm(2, MOTOR_FWD_CHANNEL1);
    config_motor_pwm(3, MOTOR_BKWD_CHANNEL0);
    config_motor_pwm(4, MOTOR_BKWD_CHANNEL1);


    /* Initialize TOF Sensor */
    sensor_data = malloc(sizeof(sensor_data_t) * NUM_OF_SENSORS);
    if (sensor_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for sensors");
        return;
    }
    vl53l0x_init(sensor_data);

    /* Configure ESP timer */
    esp_timer_create_args_t tof_timer_args = {
        .callback = &tof_sensor_callback,
        .name = "tof_sensor_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&tof_timer_args, &tof_timer));

    /* Configure Temperature Sensor */
    temperature_sensor_config_t temp_sensor = {
        .clk_src = TEMP_SENSOR_CLK_DIV,
        .range_min = TEMP_SENSOR_MIN_RANGE,
        .range_max = TEMP_SENSOR_MAX_RANGE,
    };
    init_temp_sensor(temp_sensor);

    /* NVS flash initialization */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return;
    }

    /* NimBLE host stack initialization */
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                 ret);
        return;
    }

    /* GAP service initialization */
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return;
    }

    /* GATT service initialization */
    rc = gatt_svc_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GATT service, error code: %d", rc);
        return;
    }

    /* NimBLE host configuration initialization */
    nimble_host_config_init();

    TaskHandle_t model_task;
    wrapper_load_model();
    // wrapper_run_model_test();
    esp_timer_start_periodic(tof_timer, 100000);

    /* Start all task thread and return */
    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    xTaskCreate(sensor_read_task, "sensor_read_task", 2 * SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, &sensor_task_handle);
    xTaskCreate(read_temp_task, "read_temp_task", 4 * TEMP_TASK_STACK_SIZE, NULL, TEMP_TASK_PRIORITY, &temp_read_task_handle);
    xTaskCreate(set_motor_state_task, "motor_task", MOTOR_TASK_STACK_SIZE, NULL, MOTOR_TASK_PRIORITY, &motor_task_handle);
    xTaskCreate(model_run_task, "model_run_task", 6 * 1024, NULL, 4, &model_task);

    return;
}
