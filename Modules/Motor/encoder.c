#include "encoder.h"
#include "ti_msp_dl_config.h"

Encoder_t g_encoder_left  = {0, 0.0f};
Encoder_t g_encoder_right = {0, 0.0f};

void Encoder_Init(void)
{
    // SysConfig 已配置 GPIOA 上升沿中断和优先级 0，只需使能 NVIC
    NVIC_EnableIRQ(GPIO_Encoder_INT_IRQN);
}

// GPIOA 编码器中断（MSPM0G3507 的 GPIOA 属于 GROUP1）
void GROUP1_IRQHandler(void)
{
    switch (DL_GPIO_getPendingInterrupt(GPIOA))
    {
        case GPIO_Encoder_PIN_Front_Left_A_IIDX:
            // 读 B 相判断方向：GPIOA.16
            if (DL_GPIO_readPins(GPIOA, GPIO_Encoder_PIN_Front_Left_B_PIN)) {
                g_encoder_left.count++;
            } 
            else 
            {
                g_encoder_left.count--;
            }
            break;

        case GPIO_Encoder_PIN_Front_Right_A_IIDX:
            // 读 B 相判断方向：GPIOA.17
            if (DL_GPIO_readPins(GPIOA, GPIO_Encoder_PIN_Front_Right_B_PIN)) {
                g_encoder_right.count++;
            } 
            else
            {
                g_encoder_right.count--;
            }
            break;
            
        default:
            break;
    }
}

void Encoder_UpdateSpeed(void)
{
    // 左电机
    int32_t delta_l = g_encoder_left.count;
    g_encoder_left.count = 0;
    float raw_l = PULSE_TO_M_PER_S(delta_l);
    g_encoder_left.speed_mps = FILTER_FACTOR * raw_l +
                               (1.0f - FILTER_FACTOR) * g_encoder_left.speed_mps;

    // 右电机
    int32_t delta_r = g_encoder_right.count;
    g_encoder_right.count = 0;
    float raw_r = PULSE_TO_M_PER_S(delta_r);
    g_encoder_right.speed_mps = FILTER_FACTOR * raw_r +
                                (1.0f - FILTER_FACTOR) * g_encoder_right.speed_mps;
}
