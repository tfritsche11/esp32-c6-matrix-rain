#ifndef MATRIX_RAIN_BUTTON_H
#define MATRIX_RAIN_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/* BOOT button on the Waveshare ESP32-C6-LCD-1.47 (GPIO9, active low). Only read after boot,
 * so it does not interfere with its strapping role. */
#define MATRIX_RAIN_BUTTON_GPIO 9

/* Must be called from the task that later calls matrix_rain_button_wait_press(); the ISR wakes it. */
esp_err_t matrix_rain_button_init(void);

/* Blocks up to timeout_ms; true for a debounced press, false on timeout or a bounce. */
bool matrix_rain_button_wait_press(uint32_t timeout_ms);

#endif
