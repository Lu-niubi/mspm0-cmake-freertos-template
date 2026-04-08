#include "gimbal_task.h"
#include "uc_comm.h"
#include "Step_Motor_Cmd.h"
#include "uart_printf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

QueueHandle_t xGimbalCmdQueue = NULL;

static void gimbalTask(void *pvParameters)
{
    (void)pvParameters;

    GimbalCmd_t cmd;

    while (1)
    {
        /* 阻塞等待上位机指令，超时时间无限 */
        if (xQueueReceive(xGimbalCmdQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
            /* 相对位置模式（mode=1），addr1=yaw轴，addr2=pitch轴 */
            GimbalControl(1, Step_Motor_1, cmd.yaw_delta,   500, 50);
            GimbalControl(1, Step_Motor_2, cmd.pitch_delta, 500, 50);

            uart_printf("Gimbal: yaw=%.1f pitch=%.1f\r\n",
                        cmd.yaw_delta, cmd.pitch_delta);
        }
    }
}

void Gimbal_TaskInit(void)
{
    /* 深度=1：只保留最新指令，旧指令会被覆盖 */
    xGimbalCmdQueue = xQueueCreate(1, sizeof(GimbalCmd_t));

    /* 启动UART_2中断接收（必须在队列创建后调用） */
    UC_Comm_Init();

    xTaskCreate(gimbalTask, "gimbalTask", 0x180, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
