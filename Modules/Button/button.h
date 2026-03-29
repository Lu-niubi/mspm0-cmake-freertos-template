#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

// 按键读取（低电平有效，已有上拉电阻）
// 返回 true = 按下
static inline bool Button_A26_Pressed(void)
{
    return (DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_KEY_A26_PIN) == 0);
}

static inline bool Button_A25_Pressed(void)
{
    return (DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_KEY_A25_PIN) == 0);
}

#endif // BUTTON_H
