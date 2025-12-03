#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include "init_sdcard.h"

void append_to_file(const char *filename, int moves);
void get_player_name(char *username);

#endif