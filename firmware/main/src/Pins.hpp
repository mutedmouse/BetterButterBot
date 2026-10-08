#ifndef BUTTERBOT_FIRMWARE_PINS_HPP
#define BUTTERBOT_FIRMWARE_PINS_HPP

#define PIN_BATT 6

#define PIN_PWDN 4
#define PIN_BTN 42

//XL9555 (TCA9555) expandder
#define EXP_PIN_LED 0
#define EXP_CALIB_EN 1
#define EXP_CTRL_3 6
#define EXP_CTRL_4 8
#define EXP_CTRL_5 7
#define EXP_ADDR_1 10
#define EXP_ADDR_2 11
#define EXP_ADDR_3 12
#define EXP_ADDR_4 13
#define EXP_ADDR_5 14
#define EXP_ADDR_6 15
#define EXP_DET_1 9
#define EXP_DET_2 5
#define EXP_SD_MODE_PIN 4

#define I2C_MAIN_SDA 47
#define I2C_MAIN_SCL 48

#define I2C_UMAX_SDA 41
#define I2C_UMAX_SCL 40

#define CAM_PIN_PWDN    18
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    15
#define CAM_PIN_D7      16
#define CAM_PIN_D6      14
#define CAM_PIN_D5      13
#define CAM_PIN_D4      11
#define CAM_PIN_D3      9
#define CAM_PIN_D2      7
#define CAM_PIN_D1      8
#define CAM_PIN_D0      10
#define CAM_PIN_VSYNC   21
#define CAM_PIN_HREF    17
#define CAM_PIN_PCLK    12

#define I2S_BCK_PIN        45
#define I2S_WS_PIN         5
#define I2S_DATA_OUT_PIN       3 //speaker
#define I2S_DATA_IN_PIN		   1 //mic
#define I2S_CLK_MIC            46
#define ACCELERO_INT_1 19
#define MOTOR_BRD_INT 20

#define CTRL_1 39
#define CTRL_2 38
#define CTRL_6 2

#define BATTERY_ADC 6


#endif //BUTTERBOT_FIRMWARE_PINS_HPP
