#include "joystick.h"

int main(){
    stdio_init_all();
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);

    while (1) {
        adc_select_input(1);
        uint adc_x_raw = adc_read();
        adc_select_input(2);
        uint adc_y_raw = adc_read();

        printf("X: %d\n", adc_x_raw);
        printf("Y: %d\n", adc_y_raw);
       

        sleep_ms(50);
    }

    return(0);

}