#include "matrix_rain.h"

#include <string.h>

#include "matrix_rain_font.h"

static uint32_t matrix_rain_rand(matrix_rain_t *m)
{
    /* xorshift32: tiny and deterministic for a given seed (host-testable). */
    uint32_t x = m->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m->rng = x;
    return x;
}

static void matrix_rain_spawn(matrix_rain_t *m, int col)
{
    /* Short gap before a column restarts keeps the screen dense. */
    m->head[col] = (int16_t)-(int16_t)(matrix_rain_rand(m) % MATRIX_RAIN_RESPAWN_GAP);
    m->len[col] = (uint8_t)(MATRIX_RAIN_MIN_TAIL +
                            matrix_rain_rand(m) % (MATRIX_RAIN_MAX_TAIL - MATRIX_RAIN_MIN_TAIL + 1));
    m->period[col] = (uint8_t)(1 + matrix_rain_rand(m) % 3);
    m->phase[col] = 0;
}

void matrix_rain_init(matrix_rain_t *m, uint32_t seed)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    m->rng = (seed == 0U) ? 0x9E3779B9U : seed;
    for (int col = 0; col < MATRIX_RAIN_COLS; ++col) {
        matrix_rain_spawn(m, col);
    }
}

static int matrix_rain_in_rows(int row)
{
    return row >= 0 && row < MATRIX_RAIN_ROWS;
}

static size_t matrix_rain_emit(matrix_rain_t *m, matrix_rain_cell_t *out, size_t n, size_t max,
                                   int col, int row, matrix_rain_level_t level)
{
    if (n >= max || !matrix_rain_in_rows(row)) {
        return n;
    }
    out[n].col = (uint8_t)col;
    out[n].row = (uint8_t)row;
    out[n].glyph = (level == MATRIX_RAIN_ERASE) ? 0U : m->glyph[row][col];
    out[n].level = (uint8_t)level;
    return n + 1U;
}

size_t matrix_rain_step(matrix_rain_t *m, matrix_rain_cell_t *out, size_t max)
{
    if (m == NULL || out == NULL) {
        return 0U;
    }
    size_t n = 0U;
    for (int col = 0; col < MATRIX_RAIN_COLS; ++col) {
        if (++m->phase[col] < m->period[col]) {
            continue;
        }
        m->phase[col] = 0;

        const int head = ++m->head[col];
        const int len = m->len[col];
        if (matrix_rain_in_rows(head)) {
            m->glyph[head][col] = (uint8_t)(matrix_rain_rand(m) % MATRIX_RAIN_GLYPH_COUNT);
        }
        n = matrix_rain_emit(m, out, n, max, col, head, MATRIX_RAIN_HEAD);
        n = matrix_rain_emit(m, out, n, max, col, head - 1, MATRIX_RAIN_BODY);
        if (len >= 3) {
            n = matrix_rain_emit(m, out, n, max, col, head - len + 1, MATRIX_RAIN_DIM);
        }
        n = matrix_rain_emit(m, out, n, max, col, head - len, MATRIX_RAIN_ERASE);

        /* Occasional glyph change inside the body, like cmatrix's flicker. */
        if ((matrix_rain_rand(m) & 7U) == 0U && len > 3) {
            int row = head - 2 - (int)(matrix_rain_rand(m) % (uint32_t)(len - 3));
            if (matrix_rain_in_rows(row)) {
                m->glyph[row][col] = (uint8_t)(matrix_rain_rand(m) % MATRIX_RAIN_GLYPH_COUNT);
                n = matrix_rain_emit(m, out, n, max, col, row, MATRIX_RAIN_BODY);
            }
        }

        if (head - len >= MATRIX_RAIN_ROWS - 1) {
            matrix_rain_spawn(m, col); /* tail has left the screen */
        }
    }
    return n;
}
