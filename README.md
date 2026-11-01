# esp32-c6-matrix-rain

cmatrix-style digital rain of hand-drawn 5x7 Hebrew letters for the
**Waveshare ESP32-C6-LCD-1.47**. Native ESP-IDF, no Arduino, no LVGL.

## Behavior

- Boots straight into the rain and runs forever.
- Short press on **BOOT** reseeds the rain and clears the screen.
- Onboard RGB LED: red/green/blue self-test at boot, then faint green.

Rain parameters (`main/matrix_rain.h`):

| Parameter | Value |
| --- | --- |
| Grid | 28 x 40 cells of 6 x 8 px (fills 172 x 320) |
| Glyphs | 22 Hebrew letters U+05D0..U+05EA, no final forms |
| Tail length | 12-32 cells |
| Speed | 1-3 steps per move, 80 ms per step |
| Respawn gap | up to 6 rows |
| Colors (Nord) | head Nord6, body Nord14, tail Nord3, background Nord0 |

Only changed cells are drawn each step, so there is no frame buffer.

## Hardware

- Board: Waveshare ESP32-C6-LCD-1.47 (ESP32-C6FH4, 4 MB flash)
- Display: ST7789 IPS 172 x 320 on SPI2 (MOSI 6, CLK 7, CS 14, DC 15, RST 21, BL 22, x gap 34)
- BOOT button: GPIO9, active low
- RGB LED: WS2812-type on GPIO8, byte order R, G, B

## Build and flash

Requires Nix with flakes (provides ESP-IDF v5.5.2):

```sh
nix develop
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Without Nix, any ESP-IDF >= 5.5 install works: `idf.py set-target esp32c6 && idf.py build`.

## Host test

```sh
nix develop --command make -C test test
```

## Layout

| Path | Content |
| --- | --- |
| `main/matrix_rain.{h,c}` | Rain logic (pure C, host-tested) |
| `main/matrix_rain_font.{h,c}` | 5x7 Hebrew glyph bitmaps |
| `main/matrix_rain_display.{h,c}` | ST7789 init and cell drawing |
| `main/matrix_rain_button.{h,c}` | BOOT button ISR and debounce |
| `main/matrix_rain_led.{h,c}` | RGB LED over RMT |
| `main/app_main.c` | Main loop |
| `test/` | Host test (`make test`) |

## License

MIT, see `LICENSE`.
