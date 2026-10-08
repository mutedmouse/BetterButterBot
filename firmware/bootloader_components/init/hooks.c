#include "esp_log.h"
#include <hal/gpio_hal.h>


static const char* TAG = "hook";

/* Function used to tell the linker to include this file
 * with all its symbols.
 */
void bootloader_hooks_include(void){
}


void bootloader_before_init(void) {

    ESP_LOGI(TAG, "Setting PWDN to input");

	gpio_num_t PWDN = GPIO_NUM_4;

	gpio_ll_input_enable(&GPIO, PWDN);
	gpio_ll_pulldown_dis(&GPIO, PWDN);
	gpio_ll_pullup_dis(&GPIO, PWDN);
}

void bootloader_after_init(void) {
}
