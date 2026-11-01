#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "matrix_rain.h"
#include "matrix_rain_button.h"
#include "matrix_rain_display.h"
#include "matrix_rain_led.h"

static const char *TAG = "matrix_rain";

static void matrix_rain_restart(matrix_rain_t *m)
{
    matrix_rain_init(m, esp_random());
    ESP_ERROR_CHECK(matrix_rain_display_clear());
}

void app_main(void)
{
    ESP_ERROR_CHECK(matrix_rain_display_init());

    /* LED and button are optional: the rain runs without them. */
    esp_err_t led_err = matrix_rain_led_init();
    if (led_err == ESP_OK) {
        led_err = matrix_rain_led_set(MATRIX_RAIN_LED_GREEN);
    }
    if (led_err != ESP_OK) {
        ESP_LOGW(TAG, "rgb led unavailable: %s", esp_err_to_name(led_err));
    }
    esp_err_t btn_err = matrix_rain_button_init();
    if (btn_err != ESP_OK) {
        ESP_LOGW(TAG, "button unavailable: %s", esp_err_to_name(btn_err));
    }

    static matrix_rain_t rain;
    static matrix_rain_cell_t cells[MATRIX_RAIN_MAX_CHANGES];
    matrix_rain_restart(&rain);
    ESP_LOGI(TAG, "rain started; press BOOT to reseed");

    while (true) {
        bool pressed = false;
        if (btn_err == ESP_OK) {
            pressed = matrix_rain_button_wait_press(MATRIX_RAIN_STEP_MS);
        } else {
            vTaskDelay(pdMS_TO_TICKS(MATRIX_RAIN_STEP_MS));
        }
        if (pressed) {
            matrix_rain_restart(&rain);
            ESP_LOGI(TAG, "reseeded");
            continue;
        }
        size_t n = matrix_rain_step(&rain, cells, MATRIX_RAIN_MAX_CHANGES);
        ESP_ERROR_CHECK(matrix_rain_display_draw(cells, n));
    }
}
