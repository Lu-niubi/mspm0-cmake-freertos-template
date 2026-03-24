
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ti_msp_dl_config.h"
#include "uart_printf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mpu6050_task.h"

static TaskHandle_t printLogTask_handle;
static TaskHandle_t blinkTask_handle;

void printLogTask()
{
    MPU6050_Angle angle;

    while (true)
    {
        // uart_printf("=== printLogTask thread ===\r\n");
        // uart_printf("Free heap: %d bytes\r\n", xPortGetFreeHeapSize());

        // 从队列读取最新MPU6050角度数据（不阻塞）
        if (xQueuePeek(xMPU6050AngleQueue, &angle, 0) == pdTRUE)
        {
            uart_printf("%.2f,%.2f,%.2f\r\n",
                        angle.roll, angle.pitch, angle.yaw);
        }
        else
        {
            uart_printf("MPU6050: no data yet\r\n");
        }

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
    // 0x100 = 256 words = 1 KB（uart_printf 需要更大的栈）
    xTaskCreate(printLogTask,"printLogTask",0x100,NULL,configMAX_PRIORITIES-1,&printLogTask_handle);
    // 0x80 = 128 words = 512 bytes（简单 GPIO 任务足够）
    xTaskCreate(blinkTask,"blinkTask",0x80,NULL,configMAX_PRIORITIES-1,&blinkTask_handle);
    // 初始化MPU6050任务和队列
    MPU6050_TaskInit();
    vTaskStartScheduler();
}
