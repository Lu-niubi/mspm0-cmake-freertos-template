#ifndef TRACKING_TASK_H
#define TRACKING_TASK_H

#include "FreeRTOS.h"
#include "queue.h"
#include "Tracking.h"
#include "PID.h"

// 对外暴露的队列句柄，其他任务可以从中读取最新循迹数据
extern QueueHandle_t xTrackResultQueue;   // 最新循迹结果
extern Steering_PID_Controller gSteeringPID; // 转向PID控制器（供应用层调整参数）

/**
 * @brief 创建循迹队列和任务，在 vTaskStartScheduler() 前调用
 */
void Tracking_TaskInit(void);

/**
 * @brief 循迹 FreeRTOS任务函数（由 Tracking_TaskInit 自动创建，无需手动调用）
 */
void trackingTask(void *pvParameters);

#endif // TRACKING_TASK_H
