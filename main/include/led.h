#ifndef LED_H
#define LED_H

/* Includes */
#include "driver/gpio.h"

#define BUILTIN_LED         21 //15
#define RF_SWITCH           3
#define RF_TOGGLE_GPIO      14

/* Public function declarations */
void gpio_init(void);
void led_on(void);
void led_off(void);
void rf_toggle(uint8_t state);

#endif // LED_H