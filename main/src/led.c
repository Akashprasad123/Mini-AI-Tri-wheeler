#include "led.h"
#include "common.h"
#include <esp_log.h>

#define TAG "LED"


/* Punlic functions */
void led_on(void) {
    gpio_set_level(BUILTIN_LED, 0);
    ESP_LOGI(TAG, "LED is on");
}

void led_off(void) {
    gpio_set_level(BUILTIN_LED, 1);
    ESP_LOGI(TAG, "LED is off");
}

void gpio_init(void) {
    gpio_config_t io_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BUILTIN_LED /*| 1ULL << RF_TOGGLE_GPIO | 1ULL << RF_SWITCH */,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_config);
    gpio_set_level(BUILTIN_LED, 1);
}

void rf_toggle(uint8_t state) {
    gpio_set_level(RF_SWITCH, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(RF_TOGGLE_GPIO, state);
    ESP_LOGI(TAG, "RF toggle pin toggled!");
}
