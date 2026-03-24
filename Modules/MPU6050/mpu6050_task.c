#include "mpu6050_task.h"
#include "mpu6050_kalman.h"
#include "mspm0_i2c.h"
#include "task.h"
#include "queue.h"

QueueHandle_t xMPU6050AngleQueue = NULL;
QueueHandle_t xMPU6050RawQueue   = NULL;

void mpu6050Task(void *pvParameters)
{
    (void)pvParameters;

    mpu6050_i2c_sda_unlock();

    MPU6050_InitTypeDef config = {
        .SMPLRT_Rate = 100,
        .Filter      = Band_43Hz,
        .gyro_range  = gyro_250,
        .acc_range   = acc_2g
    };
    MPU6050_Init_RawMode(&config);

    MPU6050_Angle   angle;
    MPU6050_RawData raw;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    while (1)
    {
        MPU6050_Get_Angle(&angle);
        MPU6050_Read_Raw(&raw);

        xQueueOverwrite(xMPU6050AngleQueue, &angle);
        xQueueOverwrite(xMPU6050RawQueue,   &raw);
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void MPU6050_TaskInit(void)
{
    xMPU6050AngleQueue = xQueueCreate(1, sizeof(MPU6050_Angle));
    xMPU6050RawQueue   = xQueueCreate(1, sizeof(MPU6050_RawData));

    xTaskCreate(mpu6050Task, "mpu6050Task", 0x100, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
