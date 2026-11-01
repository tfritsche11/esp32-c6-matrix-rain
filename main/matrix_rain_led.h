#ifndef MATRIX_RAIN_LED_H
#define MATRIX_RAIN_LED_H

#include <stdint.h>

#include "esp_err.h"

/* Onboard addressable RGB LED of the Waveshare ESP32-C6-LCD-1.47 (WS2812-type, data on GPIO8). */
#define MATRIX_RAIN_LED_GPIO 8

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} matrix_rain_rgb_t;

/* Faint green (full-saturation green at 3 %) while the rain runs. */
#define MATRIX_RAIN_LED_GREEN ((matrix_rain_rgb_t){0, 7, 0})

/* Initializes the RMT channel, runs a short red/green/blue self-test, then leaves the LED off. */
esp_err_t matrix_rain_led_init(void);

/* Sends one color and waits until the transfer is complete. */
esp_err_t matrix_rain_led_set(matrix_rain_rgb_t color);

#endif
