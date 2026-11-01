#include "matrix_rain_display.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "matrix_rain_font.h"

#define LCD_PIN_MOSI 6
#define LCD_PIN_CLK 7
#define LCD_PIN_CS 14
#define LCD_PIN_DC 15
#define LCD_PIN_RST 21
#define LCD_PIN_BL 22

#define LCD_H_RES 172
#define LCD_V_RES 320
#define LCD_X_GAP 34
#define LCD_Y_GAP 0

#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_SPI_TRANS_QUEUE_DEPTH 1

#define CELL_W 6
#define CELL_H 8

#define COLOR_RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8U) << 8) | (((g) & 0xFCU) << 3) | ((uint16_t)(b) >> 3)))

/* Nord palette: background, dim tail, body, head. */
#define COLOR_NORD0 COLOR_RGB565(46, 52, 64)
#define COLOR_NORD3 COLOR_RGB565(76, 86, 106)
#define COLOR_NORD6 COLOR_RGB565(236, 239, 244)
#define COLOR_NORD14 COLOR_RGB565(163, 190, 140)

static const char *TAG = "matrix_rain_display";

static esp_lcd_panel_io_handle_t s_panel_io;
static esp_lcd_panel_handle_t s_panel;
static bool s_display_ready;
static uint16_t s_line_buf[LCD_H_RES];
/* Two cell buffers alternate: with SPI queue depth 1, filling one waits for the other to be on the wire. */
static uint16_t s_cell_bufs[2][CELL_W * CELL_H];
static size_t s_cell_buf_index;

static esp_err_t matrix_rain_fill_rect(int x, int y, int width, int height, uint16_t color)
{
    if (width <= 0 || height <= 0) {
        return ESP_OK;
    }

    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = (x + width > LCD_H_RES) ? LCD_H_RES : x + width;
    int y2 = (y + height > LCD_V_RES) ? LCD_V_RES : y + height;
    if (x1 >= x2 || y1 >= y2) {
        return ESP_OK;
    }

    for (int i = 0; i < x2 - x1; i++) {
        s_line_buf[i] = color;
    }
    for (int line = y1; line < y2; line++) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(s_panel, x1, line, x2, line + 1, s_line_buf), TAG,
                            "draw line failed");
    }
    return ESP_OK;
}

esp_err_t matrix_rain_display_init(void)
{
    if (s_display_ready) {
        return ESP_OK;
    }

    gpio_config_t bl_cfg = {
        .pin_bit_mask = 1ULL << LCD_PIN_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&bl_cfg), TAG, "backlight gpio config failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(LCD_PIN_BL, 0), TAG, "backlight off failed");

    spi_bus_config_t bus_config = {
        .sclk_io_num = LCD_PIN_CLK,
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 16 * (int)sizeof(uint16_t),
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "spi init failed");

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = LCD_SPI_TRANS_QUEUE_DEPTH,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &s_panel_io),
                        TAG, "panel io init failed");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(s_panel_io, &panel_config, &s_panel), TAG, "panel init failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel hw init failed");
    /* This IPS ST7789 panel inverts pixel data by default; enable INVON so RGB565 values render as coded. */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel, true), TAG, "panel invert failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(s_panel, false), TAG, "panel swap xy failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(s_panel, false, false), TAG, "panel mirror failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(s_panel, LCD_X_GAP, LCD_Y_GAP), TAG, "panel gap failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "panel on failed");

    ESP_RETURN_ON_ERROR(gpio_set_level(LCD_PIN_BL, 1), TAG, "backlight on failed");

    s_display_ready = true;
    return matrix_rain_fill_rect(0, 0, LCD_H_RES, LCD_V_RES, COLOR_NORD0);
}

esp_err_t matrix_rain_display_clear(void)
{
    if (!s_display_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    return matrix_rain_fill_rect(0, 0, LCD_H_RES, LCD_V_RES, COLOR_NORD0);
}

/* One 6x8 cell straight to the panel: head Nord6, body Nord14, fading tail Nord3, erase Nord0. */
static esp_err_t matrix_rain_draw_cell(const matrix_rain_cell_t *c)
{
    static const uint16_t level_colors[4] = {COLOR_NORD0, COLOR_NORD3, COLOR_NORD14, COLOR_NORD6};
    uint16_t *px = s_cell_bufs[s_cell_buf_index];
    s_cell_buf_index = (s_cell_buf_index + 1U) % 2U;

    const uint16_t fg = level_colors[c->level & 3U];
    const matrix_rain_glyph_t *g = (c->level == MATRIX_RAIN_ERASE) ? NULL : matrix_rain_glyph(c->glyph);
    for (int row = 0; row < CELL_H; ++row) {
        uint8_t bits = (g != NULL && row < 7) ? g->rows[row] : 0U;
        for (int col = 0; col < CELL_W; ++col) {
            bool on = col < 5 && (bits & (0x10U >> col)) != 0U;
            px[row * CELL_W + col] = on ? fg : COLOR_NORD0;
        }
    }
    const int x = c->col * CELL_W;
    const int y = c->row * CELL_H;
    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + CELL_W, y + CELL_H, px);
}

esp_err_t matrix_rain_display_draw(const matrix_rain_cell_t *cells, size_t count)
{
    if (!s_display_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (cells == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < count; ++i) {
        ESP_RETURN_ON_ERROR(matrix_rain_draw_cell(&cells[i]), TAG, "cell draw failed");
    }
    return ESP_OK;
}
