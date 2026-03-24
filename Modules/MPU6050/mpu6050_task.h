#ifndef MPU6050_TASK_H
#define MPU6050_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include "mpu6050_kalman.h"

// 对外暴露的队列句柄，其他任务可以从中读取最新数据
extern QueueHandle_t xMPU6050AngleQueue; // 最新角度数据（roll/pitch/yaw）
extern QueueHandle_t xMPU6050RawQueue;   // 最新原始数据（加速度+陀螺仪+温度）

/**
 * @brief 创建MPU6050队列和任务，在 vTaskStartScheduler() 前调用
 */
void MPU6050_TaskInit(void);

/**
 * @brief MPU6050 FreeRTOS任务函数（由 MPU6050_TaskInit 自动创建，无需手动调用）
 */
void mpu6050Task(void *pvParameters);

#endif // MPU6050_TASK_H
