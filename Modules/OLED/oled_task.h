#ifndef OLED_TASK_H
#define OLED_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include <stdint.h>

typedef enum {
    OLED_MSG_SELECT,
    OLED_MSG_COUNTDOWN,
    OLED_MSG_RUNNING,
    OLED_MSG_DONE,
} OledMsgType_t;

typedef struct {
    OledMsgType_t type;
    uint8_t laps;        // 圈数
    uint8_t sec_left;    // 倒计时剩余秒（COUNTDOWN 用）
    uint8_t lost_count;  // 已过角数（RUNNING 用）
} OledMsg_t;

extern QueueHandle_t xOledQueue;

// 创建队列和 oledTask，在 vTaskStartScheduler() 前调用
void OLED_TaskInit(void);

#endif // OLED_TASK_H
