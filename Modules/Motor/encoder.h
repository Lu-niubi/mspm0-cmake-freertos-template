#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#define ENCODER_PPR             225         // 每转脉冲数
#define WHEEL_DIAMETER_M        0.065f      // 轮径（m）
#define SPEED_CALC_INTERVAL_MS  10          // 测速周期（ms）
#define FILTER_FACTOR           0.3f        // 低通滤波系数
#define M_PI                    3.14159265f
// 脉冲数转速度（m/s）
#define PULSE_TO_M_PER_S(pulse) \
    ((pulse) * (M_PI * WHEEL_DIAMETER_M) / \
     (ENCODER_PPR * SPEED_CALC_INTERVAL_MS * 0.001f))

typedef struct {
    volatile int32_t count;  // ISR 累加计数（有符号，正反向）
    float speed_mps;         // 低通滤波后的速度（m/s）
} Encoder_t;

extern Encoder_t g_encoder_left;
extern Encoder_t g_encoder_right;

// 启用 GPIOA 编码器中断（NVIC enable）
void Encoder_Init(void);

// 每 10ms 调用一次：读取并清零计数，计算速度（含低通滤波）
void Encoder_UpdateSpeed(void);

#endif // ENCODER_H
