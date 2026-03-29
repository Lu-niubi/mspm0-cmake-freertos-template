#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

// 初始化：启动 TIMA1 PWM 计数器，初始占空比为 0，正向
void Motor_DriverInit(void);

// 设置单路 PWM 占空比（含方向控制）
// ch: 0=左电机(CC0), 1=右电机(CC1)
// duty: -1000.0 ~ +1000.0，正值正向，负值反向（两路共享方向）
void Motor_SetPWM(uint8_t ch, float duty);

// 停止两路电机（占空比清零，方向不变）
void Motor_Stop(void);

#endif // MOTOR_DRIVER_H
