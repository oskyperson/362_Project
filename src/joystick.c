#include "joystick.h"

uint adc_x_raw;
uint adc_y_raw;

void joystick_init() {
    stdio_init_all();
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
