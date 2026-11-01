#include "matrix_rain_button.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_SETTLE_MS 30U
#define BUTTON_DEBOUNCE_MS 200

static const char *TAG = "matrix_rain_button";
static TaskHandle_t s_waiter;
static int64_t s_last_press_us = -1;

static void IRAM_ATTR matrix_rain_button_isr(void *arg)
{
    (void)arg;
    BaseType_t woken = pdFALSE;
    if (s_waiter != NULL) {
        vTaskNotifyGiveFromISR(s_waiter, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

esp_err_t matrix_rain_button_init(void)
{
    s_waiter = xTaskGetCurrentTaskHandle();

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << MATRIX_RAIN_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "gpio config failed");

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(MATRIX_RAIN_BUTTON_GPIO, matrix_rain_button_isr, NULL), TAG,
                        "isr add failed");
    return ESP_OK;
}

bool matrix_rain_button_wait_press(uint32_t timeout_ms)
{
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms)) == 0U) {
        return false;
    }

    /* Let contact bounce settle, drop the extra edges it produced, then confirm the button is held.
     * Release bounce also creates falling edges; those fail the "still low" check. */
    vTaskDelay(pdMS_TO_TICKS(BUTTON_SETTLE_MS));
    (void)ulTaskNotifyTake(pdTRUE, 0);
    if (gpio_get_level(MATRIX_RAIN_BUTTON_GPIO) != 0) {
        return false;
    }

    int64_t press_us = esp_timer_get_time();
    if (s_last_press_us >= 0 && (press_us - s_last_press_us) < (int64_t)BUTTON_DEBOUNCE_MS * 1000) {
        return false;
    }
    s_last_press_us = press_us;
    return true;
}
