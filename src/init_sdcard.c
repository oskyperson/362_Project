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

/*******************************************************************/

void init_spi_sdcard() {
    // fill in.
    spi_init(spi0, 400 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCK, GPIO_FUNC_SPI);

    gpio_init(SD_CS);
    gpio_set_dir(SD_CS, GPIO_OUT);
    gpio_put(SD_CS, true);
}

void disable_sdcard() {
    // fill in.
    uint8_t buf = 0xFF;
    gpio_put(SD_CS, true);
    spi_write16_blocking(spi0, &buf, 1);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SIO);
    gpio_put(SD_MOSI, true);

}

void enable_sdcard() {
    // fill in.
    gpio_put(SD_CS, false);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
}

void sdcard_io_high_speed() {
    // fill in.
    spi_set_baudrate(spi0, 12 * 1000000); //12MHz
}

void init_sdcard_io() {
    // fill in.
    init_spi_sdcard();
    disable_sdcard();
}

/*******************************************************************/

void init_uart();
void init_uart_irq();
void date(int argc, char *argv[]);
void command_shell();

// int main() {
//     // Initialize the standard input/output library
//     init_uart();
//     init_uart_irq();
    
//     init_sdcard_io();
    
//     // SD card functions will initialize everything.
//     command_shell();

//     for(;;);
// }