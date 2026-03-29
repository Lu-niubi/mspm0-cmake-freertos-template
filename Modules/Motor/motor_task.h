#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include "PID.h"

// 电机速度数据（发布到队列供调试/应用层读取）
typedef struct {
    float left_mps;
    float right_mps;
} MotorSpeed_t;

extern QueueHandle_t xMotorSpeedQueue;
extern Speed_PID_Controller gMotorLeftPID;
extern Speed_PID_Controller gMotorRightPID;

// 创建编码器、PWM、PID、任务和队列，在 vTaskStartScheduler() 前调用
void Motor_TaskInit(void);

void motorTask(void *pvParameters);

#endif // MOTOR_TASK_H
