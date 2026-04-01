#include "oled_task.h"
#include "oled_software_i2c.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

QueueHandle_t xOledQueue = NULL;

static void oledTask(void *pvParameters)
{
    (void)pvParameters;

    /* OLED 初始化（含 200ms 忙等延迟，必须在任务内调用） */
    OLED_Init();

    OledMsg_t msg;
    char buf[20];

    /* 等待第一条消息后开始渲染 */
    xQueueReceive(xOledQueue, &msg, portMAX_DELAY);

    while (1)
    {
        switch (msg.type)
        {
            case OLED_MSG_SELECT:
                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t *)"Select Laps:", 16);
                snprintf(buf, sizeof(buf), "  %d  lap(s)", (int)msg.laps);
                OLED_ShowString(0, 2, (uint8_t *)buf, 16);
                OLED_ShowString(0, 4, (uint8_t *)"A26+ A25- B26OK", 16);
                break;

            case OLED_MSG_COUNTDOWN:
                OLED_ShowString(0, 0, (uint8_t *)"Ready! Start in:", 16);
                snprintf(buf, sizeof(buf), "  %d lap(s)  %ds", (int)msg.laps, (int)msg.sec_left);
                OLED_ShowString(0, 2, (uint8_t *)buf, 16);
                OLED_ShowString(0, 4, (uint8_t *)"                ", 16);
                break;

            case OLED_MSG_RUNNING:
            {
                uint8_t total = msg.laps * 4;
                OLED_ShowString(0, 0, (uint8_t *)"Running...      ", 16);
                snprintf(buf, sizeof(buf), "Corner %d/%d     ", (int)msg.lost_count, (int)total);
                OLED_ShowString(0, 2, (uint8_t *)buf, 16);
                break;
            }

            case OLED_MSG_DONE:
                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t *)"Done!           ", 16);
                OLED_ShowString(0, 2, (uint8_t *)"Press B26 again ", 16);
                break;
        }

        /* 阻塞等待下一条消息 */
        xQueueReceive(xOledQueue, &msg, portMAX_DELAY);
    }
}

void OLED_TaskInit(void)
{
    xOledQueue = xQueueCreate(1, sizeof(OledMsg_t));
    xTaskCreate(oledTask, "oledTask", 0x200, NULL, 1, NULL);
}
