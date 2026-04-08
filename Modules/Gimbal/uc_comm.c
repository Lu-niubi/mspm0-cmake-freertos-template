#include "uc_comm.h"
#include "gimbal_task.h"
#include "ti_msp_dl_config.h"

#include "FreeRTOS.h"
#include "queue.h"

#define FRAME_HEAD  0xFE
#define FRAME_TAIL  0xFF
#define FRAME_LEN   6

/* 帧解析状态 */
static uint8_t s_buf[FRAME_LEN];
static uint8_t s_idx    = 0;
static uint8_t s_synced = 0;

void UC_Comm_Init(void)
{
    s_idx    = 0;
    s_synced = 0;

    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);

    /* 使能 RX 中断 */
    DL_UART_Main_enableInterrupt(UART_2_INST, DL_UART_MAIN_INTERRUPT_RX);
}

/* UART3（逻辑UART_2）中断处理 */
void UART_2_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART_2_INST))
    {
        case DL_UART_IIDX_RX:
        {
            uint8_t b = DL_UART_Main_receiveData(UART_2_INST);

            if (!s_synced)
            {
                if (b == FRAME_HEAD)
                {
                    s_buf[0] = b;
                    s_idx    = 1;
                    s_synced = 1;
                }
                break;
            }

            s_buf[s_idx++] = b;

            if (s_idx >= FRAME_LEN)
            {
                s_synced = 0;
                s_idx    = 0;

                if (s_buf[0] != FRAME_HEAD || s_buf[5] != FRAME_TAIL)
                    break;

                int16_t yaw_raw   = (int16_t)((s_buf[1] << 8) | s_buf[2]);
                int16_t pitch_raw = (int16_t)((s_buf[3] << 8) | s_buf[4]);

                float yaw   = (float)yaw_raw   / 10.0f;
                float pitch = (float)pitch_raw / 10.0f;

                if (yaw == 0.0f && pitch == 0.0f)
                    break;

                GimbalCmd_t cmd = { .yaw_delta = yaw, .pitch_delta = pitch };

                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xQueueOverwriteFromISR(xGimbalCmdQueue, &cmd, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            break;
        }

        default:
            break;
    }
}
