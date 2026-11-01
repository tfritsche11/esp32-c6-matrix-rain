#include "matrix_rain_led.h"

#include "driver/rmt_tx.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 10 MHz -> 0.1 us ticks. WS2812 bit timing: 0 = 0.3 us high / 0.9 us low, 1 = 0.9 us high / 0.3 us low. */
#define LED_RMT_RESOLUTION_HZ (10 * 1000 * 1000)
#define LED_SELFTEST_STEP_MS 400

static const char *TAG = "matrix_rain_led";
static rmt_channel_handle_t s_chan;
static rmt_encoder_handle_t s_encoder;

esp_err_t matrix_rain_led_set(matrix_rain_rgb_t color)
{
    if (s_chan == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    /* This board's LED takes red, green, blue (verified on hardware; classic WS2812 would be G, R, B). */
    uint8_t rgb[3] = {color.r, color.g, color.b};
    rmt_transmit_config_t tx_cfg = {.loop_count = 0};
    ESP_RETURN_ON_ERROR(rmt_transmit(s_chan, s_encoder, rgb, sizeof(rgb), &tx_cfg), TAG, "transmit failed");
    /* Waiting keeps rgb valid for the transfer; updates are rare, so the >50 us latch gap is implicit. */
    return rmt_tx_wait_all_done(s_chan, 100);
}

esp_err_t matrix_rain_led_init(void)
{
    rmt_tx_channel_config_t chan_cfg = {
        .gpio_num = MATRIX_RAIN_LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_RMT_RESOLUTION_HZ,
        .mem_block_symbols = 48,
        .trans_queue_depth = 1,
    };
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&chan_cfg, &s_chan), TAG, "rmt channel failed");

    rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = {.level0 = 1, .duration0 = 3, .level1 = 0, .duration1 = 9},
        .bit1 = {.level0 = 1, .duration0 = 9, .level1 = 0, .duration1 = 3},
        .flags.msb_first = 1,
    };
    ESP_RETURN_ON_ERROR(rmt_new_bytes_encoder(&enc_cfg, &s_encoder), TAG, "rmt encoder failed");
    ESP_RETURN_ON_ERROR(rmt_enable(s_chan), TAG, "rmt enable failed");

    /* Boot self-test: red, green, blue at alert brightness, then off. Confirms wiring and color order. */
    const matrix_rain_rgb_t steps[3] = {{25, 0, 0}, {0, 25, 0}, {0, 0, 25}};
    for (int i = 0; i < 3; ++i) {
        ESP_RETURN_ON_ERROR(matrix_rain_led_set(steps[i]), TAG, "self-test failed");
        vTaskDelay(pdMS_TO_TICKS(LED_SELFTEST_STEP_MS));
    }
    ESP_LOGI(TAG, "self-test done (red, green, blue)");
    return matrix_rain_led_set((matrix_rain_rgb_t){0, 0, 0});
}
