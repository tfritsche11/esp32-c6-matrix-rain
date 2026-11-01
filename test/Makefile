CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic

MAIN_DIR := ../main

.PHONY: test clean

test: test_matrix_rain
	./test_matrix_rain

test_matrix_rain: test_matrix_rain.c $(MAIN_DIR)/matrix_rain.c $(MAIN_DIR)/matrix_rain.h $(MAIN_DIR)/matrix_rain_font.c $(MAIN_DIR)/matrix_rain_font.h
	$(CC) $(CFLAGS) -I$(MAIN_DIR) test_matrix_rain.c $(MAIN_DIR)/matrix_rain.c $(MAIN_DIR)/matrix_rain_font.c -o $@

clean:
	rm -f test_matrix_rain
