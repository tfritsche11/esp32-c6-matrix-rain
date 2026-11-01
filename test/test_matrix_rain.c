#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "matrix_rain_font.h"
#include "matrix_rain.h"

int main(void)
{
    /* Font: 22 Hebrew letters (U+05D0..U+05EA without final forms), 5x7, all distinct, none blank. */
    assert(MATRIX_RAIN_GLYPH_COUNT == 22);
    for (int g = 0; g < MATRIX_RAIN_GLYPH_COUNT; ++g) {
        const matrix_rain_glyph_t *glyph = matrix_rain_glyph(g);
        assert(glyph != NULL);
        int lit = 0;
        for (int r = 0; r < 7; ++r) {
            assert((glyph->rows[r] & ~0x1Fu) == 0);
            lit += glyph->rows[r] != 0;
        }
        assert(lit > 0);
        for (int h = 0; h < g; ++h) {
            assert(memcmp(glyph, matrix_rain_glyph(h), sizeof(*glyph)) != 0);
        }
    }
    assert(matrix_rain_glyph(MATRIX_RAIN_GLYPH_COUNT) == NULL);
    assert(matrix_rain_glyph(-1) == NULL);

    /* Grid fills the 172x320 panel with 6x8 cells. */
    assert(MATRIX_RAIN_COLS == 28 && MATRIX_RAIN_ROWS == 40);

    /* Deterministic for a seed; all changes in range; simulated screen never exceeds a column's tail. */
    static matrix_rain_t a, b;
    static matrix_rain_cell_t ca[MATRIX_RAIN_MAX_CHANGES], cb[MATRIX_RAIN_MAX_CHANGES];
    static uint8_t screen[MATRIX_RAIN_ROWS][MATRIX_RAIN_COLS];
    matrix_rain_init(&a, 1234U);
    matrix_rain_init(&b, 1234U);
    memset(screen, 0, sizeof(screen));
    int heads = 0, erases = 0;
    for (int step = 0; step < 2000; ++step) {
        size_t na = matrix_rain_step(&a, ca, MATRIX_RAIN_MAX_CHANGES);
        size_t nb = matrix_rain_step(&b, cb, MATRIX_RAIN_MAX_CHANGES);
        assert(na == nb && memcmp(ca, cb, na * sizeof(ca[0])) == 0);
        assert(na <= MATRIX_RAIN_MAX_CHANGES);
        for (size_t i = 0; i < na; ++i) {
            const matrix_rain_cell_t *c = &ca[i];
            assert(c->col < MATRIX_RAIN_COLS && c->row < MATRIX_RAIN_ROWS);
            assert(c->glyph < MATRIX_RAIN_GLYPH_COUNT);
            assert(c->level <= MATRIX_RAIN_HEAD);
            heads += c->level == MATRIX_RAIN_HEAD;
            erases += c->level == MATRIX_RAIN_ERASE;
            screen[c->row][c->col] = c->level != MATRIX_RAIN_ERASE;
        }
        for (int col = 0; col < MATRIX_RAIN_COLS; ++col) {
            int lit = 0;
            for (int row = 0; row < MATRIX_RAIN_ROWS; ++row) {
                lit += screen[row][col];
            }
            assert(lit <= MATRIX_RAIN_MAX_TAIL);
        }
    }
    assert(heads > 1000 && erases > 1000); /* it actually rains, and cleans up behind itself */

    /* A different seed gives a different picture. */
    matrix_rain_init(&b, 99U);
    size_t n1 = 0, n2 = 0;
    matrix_rain_init(&a, 1234U);
    for (int i = 0; i < 30; ++i) {
        n1 += matrix_rain_step(&a, ca, MATRIX_RAIN_MAX_CHANGES);
        n2 += matrix_rain_step(&b, cb, MATRIX_RAIN_MAX_CHANGES);
    }
    assert(n1 != n2 || memcmp(ca, cb, sizeof(ca)) != 0);

    puts("PASS test_matrix_rain");
    return 0;
}
