#ifndef INIT_SDCARD_H
#define INIT_SDCARD_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************/

#define SD_MISO 16
#define SD_CS 17
#define SD_SCK 18
#define SD_MOSI 19

void init_spi_sdcard();
void disable_sdcard();
void enable_sdcard();
void sdcard_io_high_speed();
void init_sdcard_io();
#endif