#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "buzzer.h"
#include "init_sdcard.h"
#include "highscore.h"

#include "ff.h"
#include "diskio.h"
#include <string.h>


#define MOSI 15
#define SCK 14
#define CS 13
#define DC 20
#define RST 22

#define TFT_WIDTH 240
#define TFT_HEIGHT 320 //changed to 320 from 240

#define NOP 0x00
#define SWRESET 0x01
#define SLPIN 0x10
#define SLPOUT 0x11
#define NORON 0x13
#define INVOFF 0x20
#define INVON 0x21
#define DISPOFF 0x28
#define DISPON 0x29
#define CASET 0x2A
#define RASET 0x2B
#define RAMWR 0x2C
#define COLMOD 0x3A
#define MADCTL 0x36

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_BLUE 0x001F

#define MAZE_WIDTH 15
#define MAZE_HEIGHT 15

#define CELL_WIDTH (TFT_WIDTH / MAZE_WIDTH)
#define CELL_HEIGHT (TFT_HEIGHT / MAZE_HEIGHT)

#define WALL 1
#define PATH 0


#define VRX 41
#define VRY 42
#define SW 43

uint8_t maze[MAZE_HEIGHT][MAZE_WIDTH];
uint8_t maze_copy[MAZE_HEIGHT][MAZE_WIDTH];
int player_row = 1;
int player_col = 0;
int moves = 0;

uint adc_x_raw;
uint adc_y_raw;

void init_uart();
void init_uart_irq();
void date(int argc, char *argv[]);
void command_shell();

void joystick_init() {
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);
}

void joystick_read() {
    adc_select_input(1);
    adc_x_raw = adc_read();
    adc_select_input(2);
    adc_y_raw = adc_read();
}

static inline void tft_write_cmd(uint8_t cmd) {
    gpio_put(CS, 0);
    gpio_put(DC, 0);
    spi_write_blocking(spi1, &cmd, 1);
    gpio_put(CS, 1);
}

static inline void tft_write_data(uint8_t data) {
    gpio_put(CS, 0);
    gpio_put(DC, 1);
    spi_write_blocking(spi1, &data, 1);
    gpio_put(CS, 1);
}

static inline void tft_write_data_block(uint8_t *data, size_t len) {
    gpio_put(CS, 0);
    gpio_put(DC, 1);
    spi_write_blocking(spi1, data, len);
    gpio_put(CS, 1);
}

void tft_reset() {
    gpio_put(RST, 0);
    sleep_ms(100);
    gpio_put(RST, 1);
    sleep_ms(100);
}

void tft_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    tft_write_cmd(CASET);
    uint8_t x_data[] = {x1 >> 8, x1 & 0xFF, x2 >> 8, x2 & 0xFF};
    tft_write_data_block(x_data, 4);

    tft_write_cmd(RASET);
    uint8_t y_data[] = {y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF};
    tft_write_data_block(y_data, 4);

    tft_write_cmd(RAMWR);
}

void tft_init() {
    tft_reset();
    
    tft_write_cmd(SWRESET);
    sleep_ms(150);

    tft_write_cmd(SLPOUT);
    sleep_ms(255);

    tft_write_cmd(COLMOD);
    tft_write_data(0x55);
    sleep_ms(10);

    tft_write_cmd(MADCTL);
    tft_write_data(0x48); //changed to 0x48 from 0x00

    tft_write_cmd(INVON);
    sleep_ms(10);

    tft_write_cmd(NORON);
    sleep_ms(10);

    tft_write_cmd(DISPON);
    sleep_ms(255);
}

void tft_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if ((x + w) > TFT_WIDTH) w = TFT_WIDTH - x;
    if ((y + h) > TFT_HEIGHT) h = TFT_HEIGHT - y;

    tft_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    #define BUF_SIZE 512
    uint8_t buf[BUF_SIZE];
    for (int i = 0; i < BUF_SIZE; i += 2) {
        buf[i] = hi;
        buf[i + 1] = lo;
    }

    gpio_put(CS, 0);
    gpio_put(DC, 1);

    uint32_t total_pixels = w * h;
    for (uint32_t i = 0; i < total_pixels; i++) {
        if (i % (BUF_SIZE / 2) == 0) {
            uint32_t remaining = total_pixels - i;
            uint32_t chunk_len = (remaining * 2 > BUF_SIZE) ? BUF_SIZE : (remaining * 2);
            spi_write_blocking(spi1, buf, chunk_len);
        }
    }
    
    gpio_put(CS, 1);
}

void tft_fill_screen(uint16_t color) {
    tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void maze_init() {
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            maze[y][x] = WALL;
        }
    }
}

void shuffle(int *array, size_t n) {
    if (n > 1) {
        for (size_t i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void carve_maze(int x, int y) {
    maze[y][x] = PATH;

    int dirs[4][2] = {{0, -2}, {0, 2}, {-2, 0}, {2, 0}};
    int dir_indices[] = {0, 1, 2, 3};
    
    shuffle(dir_indices, 4);

    for (int i = 0; i < 4; i++) {
        int dir_index = dir_indices[i];
        
        int nx = x + dirs[dir_index][0];
        int ny = y + dirs[dir_index][1];

        if (nx > 0 && nx < MAZE_WIDTH - 1 && ny > 0 && ny < MAZE_HEIGHT - 1 && maze[ny][nx] == WALL) {
            int wall_x = x + dirs[dir_index][0] / 2;
            int wall_y = y + dirs[dir_index][1] / 2;
            maze[wall_y][wall_x] = PATH;

            carve_maze(nx, ny);
        }
    }
}

void draw_maze() {
    tft_set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    
    gpio_put(CS, 0);
    gpio_put(DC, 1);

    for (int y = 0; y < TFT_HEIGHT; y++) {
        for (int x = 0; x < TFT_WIDTH; x++) {
            int maze_x = x / CELL_WIDTH;
            int maze_y = y / CELL_HEIGHT;

            uint16_t color;
            if (maze_x >= MAZE_WIDTH || maze_y >= MAZE_HEIGHT) {
                color = COLOR_BLACK; 
            } else {
                color = (maze[maze_y][maze_x] == WALL) ? COLOR_WHITE : COLOR_BLACK;
            }

            uint8_t pixel_data[] = {color >> 8, color & 0xFF};
            spi_write_blocking(spi1, pixel_data, 2);
        }
    }

    gpio_put(CS, 1);
}

void draw_player() {
    int new_row = player_row;
    int new_col = player_col;

    // Values go from 0 to 4096, 2048 is the center
    if(adc_x_raw < 1900) new_col--;
    else if(adc_x_raw > 2200) new_col++;
    if(adc_y_raw < 1900) new_row--;
    else if(adc_y_raw > 2200) new_row++;

    // Check if move is valid
    if(new_row > 0 && new_row < MAZE_HEIGHT && new_col > 0 && new_col < MAZE_WIDTH && (new_row != player_row || new_col != player_col)) {
        if(maze[new_row][new_col] == WALL) {
            buzzer_play_tone(1000, 100);
        } else {

            tft_fill_rect(player_col * CELL_WIDTH, player_row * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, COLOR_BLUE); 

            player_row = new_row;
            player_col = new_col;
            moves++;

            tft_fill_rect(player_col * CELL_WIDTH, player_row * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, COLOR_RED);
        }
    }
}

char name [20];
int main() {
    init_uart();
    init_uart_irq();
    
    init_sdcard_io();
    
    // SD card functions will initialize everything.
    command_shell();

    stdio_init_all();
    sleep_ms(1000);
    printf("Initializing\n");

    /*------ SD CARD LOADING STUFF -----*/
    DSTATUS stat = disk_initialize(1);
    if(stat != 0){
        printf("UGH SD NO WORK: %d", stat);
    }

    //readMaze(1);

    memcpy(maze, maze_copy, sizeof(int) * MAZE_HEIGHT * MAZE_WIDTH);


    fflush(stdout); 

    spi_init(spi1, 10 * 1000 * 1000); //changed
    gpio_set_function(SCK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1);

    gpio_init(DC);
    gpio_set_dir(DC, GPIO_OUT);

    gpio_init(RST);
    gpio_set_dir(RST, GPIO_OUT);

    buzzer_init(35); // change whatever the buzzer pin is

    printf("SPI and GPIO initialized\n");

    tft_init();
    printf("TFT Initialized\n");
    tft_fill_screen(COLOR_BLUE);
    sleep_ms(500);

    printf("Seeding RNG\n");
    //uint64_t seed = to_us_since_boot(get_absolute_time());
    //srand(seed);
    srand(42); // try making a static maze. Tested the code and worked fine with preset seed

    printf("Initializing maze grid\n");
    maze_init();

    printf("Generating maze\n");
    carve_maze(1, 1);
    maze[1][0] = PATH; //START
    maze[MAZE_HEIGHT - 2][MAZE_WIDTH - 1] = PATH; // END

    printf("Drawing maze\n");
    draw_maze();

    printf("Done\n");

    tft_fill_rect(player_col * CELL_WIDTH, player_row * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, COLOR_RED);
    joystick_init();

    while (player_col != MAZE_WIDTH - 1 || player_row != MAZE_HEIGHT - 2) {
        tight_loop_contents();
        joystick_read();
        draw_player();
        buzzer_update();
        sleep_ms(200);
        
    }

    tft_fill_screen(COLOR_RED);

    const uint32_t freqs[] = {523, 659, 784, 988, 1046};
    const uint32_t durs[]  = {80, 80, 80, 80, 140};

    for (int i = 0; i < 5; i++) {
        buzzer_play_tone(freqs[i], durs[i]);
        sleep_ms(durs[i]);
        buzzer_update();
    }
    
    
    //get_player_name(name);
    append_to_file("score1.txt", moves);

    return 0;
}