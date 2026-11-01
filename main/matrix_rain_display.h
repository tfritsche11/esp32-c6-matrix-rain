#ifndef MATRIX_RAIN_DISPLAY_H
#define MATRIX_RAIN_DISPLAY_H

#include <stddef.h>

#include "esp_err.h"
#include "matrix_rain.h"

/* ST7789 IPS LCD (172x320) of the Waveshare ESP32-C6-LCD-1.47 on SPI2. */
esp_err_t matrix_rain_display_init(void);

/* Fills the whole panel with the background color. */
esp_err_t matrix_rain_display_clear(void);

/* Draws the changed cells of one step straight to the panel. */
esp_err_t matrix_rain_display_draw(const matrix_rain_cell_t *cells, size_t count);

#endif
