#include "GPIOPeriph.h"

void GPIOPeriph::setMode(gpio_num_t gpioNum, GPIOMode mode){
	gpio_reset_pin(gpioNum);
	switch(mode){
		case GPIOMode::Input:
			gpio_set_direction(gpioNum, GPIO_MODE_INPUT);
			break;
		case GPIOMode::Output:
			gpio_set_direction(gpioNum, GPIO_MODE_OUTPUT);
			break;
	}

}

void GPIOPeriph::write(gpio_num_t gpioNum, bool value){
	gpio_set_level(gpioNum, value);
}

bool GPIOPeriph::read(gpio_num_t gpioNum){
	return gpio_get_level(gpioNum);
}

void GPIOPeriph::setPullup(gpio_num_t gpioNum, bool pulledUp){
	gpio_set_pull_mode(gpioNum, pulledUp ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
}

void GPIOPeriph::setPulldown(gpio_num_t gpioNum, bool pulledDown){
	gpio_set_pull_mode(gpioNum, pulledDown ? GPIO_PULLDOWN_ONLY : GPIO_FLOATING);
}
