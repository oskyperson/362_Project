#include "hardware/uart.h" 
#include "hardware/gpio.h" 
#include <stdio.h>  
#include <string.h>
#include "pico/stdlib.h"
#include "sdcard.h"

#define BUFSIZE 100
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

// add this here so that compiler does not complain about implicit function
// in init_uart_irq
void uart_rx_handler();

/*******************************************************************/

void init_uart() {
    // fill in
    uart_init(uart0, 115200);
    gpio_set_function(0, UART_FUNCSEL_NUM(0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(0, 1));
    uart_set_baudrate(uart0, 115200);
    uart_set_format (uart0, 8, 1, UART_PARITY_NONE);
}

void init_uart_irq() {
    // uart_set_fifo_enabled (uart0, false);
    // int UART_IRQ = UART0_IRQ;
    // uart0_hw->imsc = 1 << UART_UARTIMSC_RXIM_LSB;
    // irq_set_exclusive_handler(UART_IRQ, uart_rx_handler);
    // irq_set_enabled(UART_IRQ, true);
    // uart_set_irq_enables(uart0, true, false);

    uart_set_fifo_enabled(uart0, false);
    uart_set_irq_enables(uart0, true, false);
    irq_set_exclusive_handler(UART0_IRQ, uart_rx_handler);
    irq_set_enabled(UART0_IRQ, true);
}

void uart_rx_handler() {
    // fill in.
    // hw_clear_bits(&uart_get_hw(uart0)->icr, 1u << 0);
    // if(seridx >= BUFSIZE){
    //     return;
    // }
    // char c = uart0_hw->dr;
    // if (c == 0x0A){
    //     newline_seen = 1;
    // }

    // if(c == 8){
    //     uart_putc(uart0, 8);
    //     uart_putc(uart0, 32);
    //     uart_putc(uart0, 8);
    //     seridx = seridx > 0? seridx - 1 : seridx;
    //     serbuf[seridx] = '\0';
    //     return;
    // }
    // else{
    //     printf("%c", c);
    //     serbuf[seridx++] = c;
    // }

    uart_get_hw(uart0)->icr = 33;

    if (seridx >= BUFSIZE)
        return;

    char d = uart_get_hw(uart0)->dr;  

    if (d == '\n') {
        newline_seen = 1;
    }

    if (d == 8 && seridx > 0) {
        uart_putc(uart0, 8);  
        uart_putc(uart0, 32);
        uart_putc(uart0, 8);  
        seridx--;              
        serbuf[seridx] = '\0';
    } else {
        uart_putc(uart0, d);       
        serbuf[seridx] = d; 
        seridx++; 
    }
}

int _read(__unused int handle, char *buffer, int length) {
    // fill in.
    while(newline_seen == 0){
        sleep_ms(5);
    }
    if(newline_seen == 1){
        newline_seen = 0;
    }

    int itr = length >= seridx ? seridx : length;

    for (int i = 0; i < itr; i++) {
        buffer[i] = serbuf[i]; 
    }
    seridx = 0;

    return length;
}

int _write(int handle, char *buffer, int length) {
    // for (int i = 0; i < length; i++){
    //     if (buffer[i] != '\0'){
    //        uart_putc(uart0, buffer[i]);
    //     }
    //     else{
    //         break;
    //     }
    // }

    for (int i = 0; i < length; i++) {
        uart_putc(uart0, buffer[i]); 
    }
    return length;
}

/*******************************************************************/

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

struct commands_t cmds[] = {
        { "append", append },
        { "cat", cat },
        { "cd", cd },
        { "date", date },
        { "input", input },
        { "ls", ls },
        { "mkdir", mkdir },
        { "mount", mount },
        { "pwd", pwd },
        { "rm", rm },
        { "restart", restart }
};

// This function inserts a string into the input buffer and echoes it to the UART
// but whatever is "typed" by this function can be edited by the user.
void insert_echo_string(const char* str) {
    // Print the string and copy it into serbuf, allowing editing
    seridx = 0;
    newline_seen = 0;
    memset(serbuf, 0, BUFSIZE);

    // Print and fill serbuf with the initial string
    for (int i = 0; str[i] != '\0' && seridx < BUFSIZE - 1; i++) {
        char c = str[i];
        uart_write_blocking(uart0, (uint8_t*)&c, 1);
        serbuf[seridx++] = c;
    }
}

void parse_command(const char* input) {
    char *token = strtok(input, " ");
    int argc = 0;
    char *argv[10];
    while (token != NULL && argc < 10) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    int i = 0;
    for(; i<sizeof cmds/sizeof cmds[0]; i++) {
        if (strcmp(cmds[i].cmd, argv[0]) == 0) {
            cmds[i].fn(argc, argv);
            break;
        }
    }
    if (i == (sizeof cmds/sizeof cmds[0])) {
        printf("Unknown command: %s\n", argv[0]);
    }
}

void command_shell() {
    char input[100];
    memset(input, 0, sizeof(input));

    // Disable buffering for stdout
    setbuf(stdout, NULL);

    printf("\nEnter current ");
    insert_echo_string("date 20250701120000");
    fgets(input, 99, stdin);
    input[strcspn(input, "\r")] = 0; // Remove CR character
    input[strcspn(input, "\n")] = 0; // Remove newline character
    parse_command(input);
    
    printf("SD Card Command Shell");
    printf("\r\nType 'mount' to mount the SD card.\n");
    while (1) {
        printf("\r\n> ");
        fgets(input, sizeof(input), stdin);
        fflush(stdin);
        input[strcspn(input, "\r")] = 0; // Remove CR character
        input[strcspn(input, "\n")] = 0; // Remove newline character
        
        parse_command(input);
    }
}