#include "tracking_task.h"
#include "Tracking.h"
#include "PID.h"
#include "task.h"
#include "queue.h"

QueueHandle_t xTrackResultQueue = NULL;
Steering_PID_Controller gSteeringPID;

void trackingTask(void *pvParameters)
{
    (void)pvParameters;

    TrackSensorData sensor_data;
    TrackResult result;

    // 初始化转向PID控制器
    // 参数：Kp, Ki, Kd, output_limit, filter_alpha
    Steering_PID_Init(&gSteeringPID, 0.03f, 0.0f, 0.0f, 0.2f, 0.9f);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    while (1)
    {
        // 读取8路传感器
        Track_ReadSensors(&sensor_data);

        // 计算加权平均循迹结果
        result = CalculateTrackResult(&sensor_data);

        // 推入队列供其他任务读取
        xQueueOverwrite(xTrackResultQueue, &result);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void Tracking_TaskInit(void)
{
    xTrackResultQueue = xQueueCreate(1, sizeof(TrackResult));

    xTaskCreate(trackingTask, "trackingTask", 0x100, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
