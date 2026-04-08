#ifndef GIMBAL_TASK_H
#define GIMBAL_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include <stdint.h>

/* 上位机发来的云台控制指令 */
typedef struct {
    float yaw_delta;    /* yaw增量，单位°  */
    float pitch_delta;  /* pitch增量，单位° */
} GimbalCmd_t;

/* 中断→任务通信队列（深度=1，只保留最新指令） */
extern QueueHandle_t xGimbalCmdQueue;

/**
 * @brief 初始化云台模块：创建队列、启动UART_2接收中断、创建FreeRTOS任务
 */
void Gimbal_TaskInit(void);

#endif /* GIMBAL_TASK_H */
