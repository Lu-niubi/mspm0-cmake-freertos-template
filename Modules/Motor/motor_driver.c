#include "motor_driver.h"
#include "mpu6050_kalman.h"
// 正转：AIN1=0, AIN2=1, BIN1=1, BIN2=0
static void Motor_Normal(void)
{
    DL_GPIO_clearPins(GPIO_IN_GPIO_AIN1_PORT, GPIO_IN_GPIO_AIN1_PIN);
    DL_GPIO_setPins(GPIO_IN_GPIO_AIN2_PORT,   GPIO_IN_GPIO_AIN2_PIN);
    DL_GPIO_setPins(GPIO_IN_GPIO_BIN1_PORT,   GPIO_IN_GPIO_BIN1_PIN);
    DL_GPIO_clearPins(GPIO_IN_GPIO_BIN2_PORT, GPIO_IN_GPIO_BIN2_PIN);
}

// 反转：AIN1=1, AIN2=0, BIN1=0, BIN2=1
static void Motor_Reverse(void)
{
    DL_GPIO_setPins(GPIO_IN_GPIO_AIN1_PORT,   GPIO_IN_GPIO_AIN1_PIN);
    DL_GPIO_clearPins(GPIO_IN_GPIO_AIN2_PORT, GPIO_IN_GPIO_AIN2_PIN);
    DL_GPIO_clearPins(GPIO_IN_GPIO_BIN1_PORT, GPIO_IN_GPIO_BIN1_PIN);
    DL_GPIO_setPins(GPIO_IN_GPIO_BIN2_PORT,   GPIO_IN_GPIO_BIN2_PIN);
}

void Motor_DriverInit(void)
{
    DL_Timer_startCounter(PWM_0_INST);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
    Motor_Normal();
}

void Motor_SetPWM(uint8_t ch, float duty)
{
    // 限幅
    if (duty > 1000.0f)  duty = 1000.0f;
    if (duty < -1000.0f) duty = -1000.0f;

    // 方向控制（两路共享，与 Keil 原版行为一致）
    if (duty >= 0.0f) 
    {
        Motor_Normal();
    } 
    else 
    {
        Motor_Reverse();
    }

    uint32_t ccr = (uint32_t)mpu_fabsf(duty);
    if (ch == 0)
    {
        DL_Timer_setCaptureCompareValue(PWM_0_INST, ccr, GPIO_PWM_0_C0_IDX);
    }
    else
    {
        DL_Timer_setCaptureCompareValue(PWM_0_INST, ccr, GPIO_PWM_0_C1_IDX);
    }
}

void Motor_Stop(void)
{
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
}
