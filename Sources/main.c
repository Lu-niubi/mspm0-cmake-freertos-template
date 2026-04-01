
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ti_msp_dl_config.h"
#include "uart_printf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mpu6050_task.h"
#include "tracking_task.h"
#include "motor_task.h"
#include "oled_task.h"

static TaskHandle_t printLogTask_handle;
static TaskHandle_t blinkTask_handle;

void printLogTask()
{
    MPU6050_Angle angle;
    TrackResult track;
    MotorSpeed_t mspeed;

    while (true)
    {
        // // 从队列读取最新MPU6050角度数据（不阻塞）
        // if (xQueuePeek(xMPU6050AngleQueue, &angle, 0) == pdTRUE)
        // {
        //     uart_printf("MPU6050: R=%.2f P=%.2f Y=%.2f\r\n",
        //                 angle.roll, angle.pitch, angle.yaw);
        // }
        // else
        // {
        //     uart_printf("MPU6050: no data yet\r\n");
        // }

        // 从队列读取最新循迹数据（不阻塞）
        if (xQueuePeek(xTrackResultQueue, &track, 0) == pdTRUE)
        {
            const char *state_str[] = {"LEFT", "RIGHT", "CENTER", "LOST"};
            uart_printf("Track: pos=%.2f err=%.2f state=%s pattern=0x%02X\r\n",
                        track.line_position, track.track_error,
                        state_str[track.state], track.sensor_pattern);
        }
        else
        {
            uart_printf("Track: no data yet\r\n");
        }

        uart_printf("---\r\n");

        // 从队列读取最新电机速度数据（不阻塞）
        if (xQueuePeek(xMotorSpeedQueue, &mspeed, 0) == pdTRUE)
        {
            uart_printf("Motor: L=%.3f R=%.3f m/s\r\n",
                        mspeed.left_mps, mspeed.right_mps);
        }
        else
        {
            uart_printf("Motor: no data yet\r\n");
        }

        uart_printf("===\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void blinkTask()
{
    while (true)
    {
        // LED 5Hz频率闪烁
        DL_GPIO_togglePins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);
        DL_GPIO_togglePins(PORTA_PORT,PORTA_LED_USER_PIN);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    while(1);
}

int main()
{
    SYSCFG_DL_init();
    // 0x200 = 512 words = 2 KB（uart_printf + 循迹打印需要更大的栈）
    xTaskCreate(printLogTask,"printLogTask",0x200,NULL,configMAX_PRIORITIES-1,&printLogTask_handle);
    // 0x80 = 128 words = 512 bytes（简单 GPIO 任务足够）
    xTaskCreate(blinkTask,"blinkTask",0x80,NULL,configMAX_PRIORITIES-1,&blinkTask_handle);
    // // 初始化MPU6050任务和队列
    // MPU6050_TaskInit();
    // 初始化循迹任务和队列
    Tracking_TaskInit();
    // 初始化 OLED 任务和队列（必须在 Motor_TaskInit 之前，确保 xOledQueue 先创建）
    OLED_TaskInit();
    // 初始化电机任务（编码器+PWM+PID）
    Motor_TaskInit();
    vTaskStartScheduler();
}
