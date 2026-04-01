#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include "PID.h"
#include "oled_task.h"

typedef struct {
    float left_mps;
    float right_mps;
} MotorSpeed_t;

extern QueueHandle_t xMotorSpeedQueue;
extern Speed_PID_Controller gMotorLeftPID;
extern Speed_PID_Controller gMotorRightPID;
extern float g_base_speed;

void Motor_TaskInit(void);
void motorTask(void *pvParameters);

#endif // MOTOR_TASK_H
