#include "motor_task.h"
#include "motor_driver.h"
#include "encoder.h"
#include "task.h"
#include "queue.h"

QueueHandle_t xMotorSpeedQueue = NULL;
Speed_PID_Controller gMotorLeftPID;
Speed_PID_Controller gMotorRightPID;

void motorTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    while (1)
    {
        // 1. 计算本周期速度（读计数、清零、低通滤波）
        Encoder_UpdateSpeed();

        // 2. PID 计算输出（dt = 10ms = 0.01s）
        // target_speed 存在 gMotorLeftPID 的自定义字段中（通过外部直接写 pid.Kp 等调参后，
        // 这里用全局变量 g_target_left/right 更清晰，当前阶段 PID 参数=0 输出为 0）
        float duty_l = Speed_PID_Compute(&gMotorLeftPID,
                                         0.5f,
                                         g_encoder_left.speed_mps, 0.01f);
        float duty_r = Speed_PID_Compute(&gMotorRightPID,
                                         0.5f,
                                         g_encoder_right.speed_mps, 0.01f);

        // 3. 输出 PWM
        Motor_SetPWM(0, duty_l);
        Motor_SetPWM(1, duty_r);

        // 4. 发布速度到队列
        MotorSpeed_t speeds = {
            .left_mps  = g_encoder_left.speed_mps,
            .right_mps = g_encoder_right.speed_mps,
        };
        xQueueOverwrite(xMotorSpeedQueue, &speeds);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void Motor_TaskInit(void)
{
    // 初始化硬件
    Encoder_Init();
    Motor_DriverInit();

    // 初始化速度 PID（Kp=Ki=Kd=0，先验证测速，再调参）
    Speed_PID_Init(&gMotorLeftPID,  1500.0f, 0.0f, 0.0f, 1000.0f, 0.6f);
    Speed_PID_Init(&gMotorRightPID, 1500.0f, 0.0f, 0.0f, 1000.0f, 0.6f);

    xMotorSpeedQueue = xQueueCreate(1, sizeof(MotorSpeed_t));

    xTaskCreate(motorTask, "motorTask", 0x200, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
