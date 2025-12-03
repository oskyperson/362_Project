// #include "pico/stdlib.h"
// #include "hardware/spi.h"
// #include <stdio.h>

// #define SD_CS   17
// #define SD_MISO 16
// #define SD_MOSI 19
// #define SD_SCK  18

// int main() {
//     stdio_init_all();
//     sleep_ms(2000); // wait for serial monitor

//     printf("Minimal SD SPI test\n");

//     // SPI init
//     spi_init(spi0, 400 * 1000); // slow clock for init
//     gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
//     gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);
//     gpio_set_function(SD_SCK,  GPIO_FUNC_SPI);

//     // CS as GPIO
//     gpio_init(SD_CS);
//     gpio_set_dir(SD_CS, true); // output
//     gpio_put(SD_CS, true);     // CS high

//     // Send 80 dummy clocks (10 bytes of 0xFF)
//     uint8_t dummy = 0xFF;
//     for (int i = 0; i < 10; i++) {
//         spi_write_blocking(spi0, &dummy, 1);
//     }

//     // CMD0: GO_IDLE_STATE
//     gpio_put(SD_CS, false);    // CS low to select card
//     uint8_t cmd0[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
//     uint8_t response[6] = {0};
//     spi_write_read_blocking(spi0, cmd0, response, 6);

//     // Print response bytes
//     printf("CMD0 response: ");
//     for (int i = 0; i < 6; i++) {
//         printf("0x%02X ", response[i]);
//     }
//     printf("\n");

//     // Deselect card
//     gpio_put(SD_CS, true);
//     spi_write_blocking(spi0, &dummy, 1); // extra 8 clocks

//     while (1) {
//         tight_loop_contents();
//     }
// }
