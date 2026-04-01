#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

/*
 * 消抖：采用边沿检测，调用方需以固定周期（20ms）调用 Button_Scan()。
 * A26 / A25 在 GPIOA，B26 在 GPIOB，均为上拉输入，低电平有效。
 */

typedef struct {
    bool a26_edge;  // A26 下降沿（+1圈）
    bool a25_edge;  // A25 下降沿（-1圈）
    bool b26_edge;  // B26 下降沿（确认启动）
} ButtonEvent_t;

static inline ButtonEvent_t Button_Scan(void)
{
    static bool prev_a26 = false;
    static bool prev_a25 = false;
    static bool prev_b26 = false;

    bool cur_a26 = (DL_GPIO_readPins(GPIO_KEY_KEY_A26_PORT, GPIO_KEY_KEY_A26_PIN) == 0);
    bool cur_a25 = (DL_GPIO_readPins(GPIO_KEY_KEY_A25_PORT, GPIO_KEY_KEY_A25_PIN) == 0);
    bool cur_b26 = (DL_GPIO_readPins(GPIO_KEY_KEY_B26_PORT, GPIO_KEY_KEY_B26_PIN) == 0);

    ButtonEvent_t ev = {
        .a26_edge = (cur_a26 && !prev_a26),
        .a25_edge = (cur_a25 && !prev_a25),
        .b26_edge = (cur_b26 && !prev_b26),
    };

    prev_a26 = cur_a26;
    prev_a25 = cur_a25;
    prev_b26 = cur_b26;

    return ev;
}

#endif /* BUTTON_H */
