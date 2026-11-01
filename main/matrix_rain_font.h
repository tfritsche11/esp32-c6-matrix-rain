#ifndef MATRIX_RAIN_FONT_H
#define MATRIX_RAIN_FONT_H

#include <stdint.h>

/* Hebrew letters U+05D0..U+05EA (alef..tav, no final forms) as 5x7 bitmaps for the matrix rain. */
#define MATRIX_RAIN_GLYPH_COUNT 22

typedef struct {
    uint8_t rows[7]; /* bit 4 = leftmost pixel */
} matrix_rain_glyph_t;

const matrix_rain_glyph_t *matrix_rain_glyph(int index);

#endif
