#ifndef __OLED_SOFTWARE_I2C_H
#define __OLED_SOFTWARE_I2C_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

#define OLED_CMD  0
#define OLED_DATA 1

#ifndef GPIO_OLED_PIN_OLED_SCL_PORT
#define GPIO_OLED_PIN_OLED_SCL_PORT GPIO_OLED_PORT
#endif
#ifndef GPIO_OLED_PIN_OLED_SDA_PORT
#define GPIO_OLED_PIN_OLED_SDA_PORT GPIO_OLED_PORT
#endif

#define OLED_SCL_Set()  (DL_GPIO_setPins(GPIO_OLED_PIN_OLED_SCL_PORT, GPIO_OLED_PIN_OLED_SCL_PIN))
#define OLED_SCL_Clr()  (DL_GPIO_clearPins(GPIO_OLED_PIN_OLED_SCL_PORT, GPIO_OLED_PIN_OLED_SCL_PIN))
#define OLED_SDA_Set()  (DL_GPIO_setPins(GPIO_OLED_PIN_OLED_SDA_PORT, GPIO_OLED_PIN_OLED_SDA_PIN))
#define OLED_SDA_Clr()  (DL_GPIO_clearPins(GPIO_OLED_PIN_OLED_SDA_PORT, GPIO_OLED_PIN_OLED_SDA_PIN))

void OLED_WR_Byte(uint8_t dat, uint8_t cmd);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t sizey);
uint32_t oled_pow(uint8_t m, uint8_t n);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t sizey);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len1, uint8_t len2, uint8_t sizey);
void OLED_Init(void);

#endif
