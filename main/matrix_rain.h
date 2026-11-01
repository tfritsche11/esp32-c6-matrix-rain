#ifndef MATRIX_RAIN_H
#define MATRIX_RAIN_H

#include <stddef.h>
#include <stdint.h>

/* cmatrix-style rain on a 28x40 grid of 6x8 cells (fills 172x320). Pure logic: each step returns only
 * the cells that change, so the display can draw a few glyphs instead of a full frame. */
#define MATRIX_RAIN_COLS 28
#define MATRIX_RAIN_ROWS 40
#define MATRIX_RAIN_MIN_TAIL 12
#define MATRIX_RAIN_MAX_TAIL 32
#define MATRIX_RAIN_STEP_MS 80U
/* Max rows a column waits (off-screen) before a new drop enters. */
#define MATRIX_RAIN_RESPAWN_GAP 6
/* Per moving column: head, body, dim, erase, plus one random glyph flicker. */
#define MATRIX_RAIN_MAX_CHANGES (MATRIX_RAIN_COLS * 5)

typedef enum {
    MATRIX_RAIN_ERASE = 0,
    MATRIX_RAIN_DIM,
    MATRIX_RAIN_BODY,
    MATRIX_RAIN_HEAD,
} matrix_rain_level_t;

typedef struct {
    uint8_t col;
    uint8_t row;
    uint8_t glyph; /* index into matrix_rain_glyph() */
    uint8_t level; /* matrix_rain_level_t */
} matrix_rain_cell_t;

typedef struct {
    int16_t head[MATRIX_RAIN_COLS];  /* row of the drop head; negative = still waiting to enter */
    uint8_t len[MATRIX_RAIN_COLS];   /* tail length in cells */
    uint8_t period[MATRIX_RAIN_COLS]; /* steps per move: 1 fast .. 3 slow */
    uint8_t phase[MATRIX_RAIN_COLS];
    uint8_t glyph[MATRIX_RAIN_ROWS][MATRIX_RAIN_COLS];
    uint32_t rng;
} matrix_rain_t;

void matrix_rain_init(matrix_rain_t *m, uint32_t seed);
/* Advances one step and writes at most max changed cells to out; returns the count. */
size_t matrix_rain_step(matrix_rain_t *m, matrix_rain_cell_t *out, size_t max);

#endif
