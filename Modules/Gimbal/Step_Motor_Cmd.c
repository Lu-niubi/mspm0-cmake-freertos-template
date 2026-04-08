#include "Step_Motor_Cmd.h"

static float my_fabsf(float x) { return (x < 0.0f) ? -x : x; }

/* 记录两路电机累计脉冲数（用于绝对位置模式） */
static int32_t motor1_position = 0;
static int32_t motor2_position = 0;

static uint8_t cmd[16] = {0};

static void Send_Array(uint8_t addr, const uint8_t *array, uint16_t length)
{
    if (array == NULL)
        return;

    if (addr == 1)
    {
        for (uint16_t i = 0; i < length; i++)
        {
            while (DL_UART_isBusy(UART_0_INST) == true);
            DL_UART_Main_transmitData(UART_0_INST, array[i]);
        }
    }
    else if (addr == 2)
    {
        for (uint16_t i = 0; i < length; i++)
        {
            while (DL_UART_isBusy(UART_1_INST) == true);
            DL_UART_Main_transmitData(UART_1_INST, array[i]);
        }
    }
}

/* 角度 -> 脉冲数（3200脉冲/转） */
static int32_t angle_to_pulse(float degrees)
{
    return (int32_t)(degrees * 3200 / 360.0f + 0.5f);
}

/* 脉冲数 -> 角度（0-360°） */
static float pulse_to_angle(int32_t pulse)
{
    float angle = (pulse % 3200) * 360.0f / 3200.0f;
    return angle < 0 ? angle + 360.0f : angle;
}

/**
 * @brief 位置模式底层命令（Emm42协议）
 */
void Emm_Position(uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc,
                  uint32_t clk, bool raF, bool snF)
{
    cmd[0]  = addr;
    cmd[1]  = 0xFD;
    cmd[2]  = dir;
    cmd[3]  = (uint8_t)(vel >> 8);
    cmd[4]  = (uint8_t)(vel >> 0);
    cmd[5]  = acc;
    cmd[6]  = (uint8_t)(clk >> 24);
    cmd[7]  = (uint8_t)(clk >> 16);
    cmd[8]  = (uint8_t)(clk >> 8);
    cmd[9]  = (uint8_t)(clk >> 0);
    cmd[10] = raF;
    cmd[11] = snF;
    cmd[12] = 0x6B;
    Send_Array(addr, cmd, 13);
}

/**
 * @brief 绝对位置模式（0-360°，自动选最短路径）
 */
void AbsolutePositionMode(uint8_t addr, float target_angle, uint16_t vel, uint8_t acc)
{
    float current_angle;
    if (addr == 1)       current_angle = pulse_to_angle(motor1_position);
    else if (addr == 2)  current_angle = pulse_to_angle(motor2_position);
    else return;

    float diff = target_angle - current_angle;
    if (diff >  180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;

    int32_t rel_pulse = angle_to_pulse(my_fabsf(diff));
    uint8_t dir = (diff >= 0) ? 0 : 1;  /* 0=CW, 1=CCW */

    Emm_Position(addr, dir, vel, acc, rel_pulse, false, false);

    int32_t *pos_ptr = (addr == 1) ? &motor1_position : &motor2_position;
    *pos_ptr += (dir ? -rel_pulse : rel_pulse);
}

/**
 * @brief 相对位置模式（+顺时针，-逆时针）
 */
void RelativePositionMode(uint8_t addr, float rel_angle, uint16_t vel, uint8_t acc)
{
    int32_t pulse = angle_to_pulse(my_fabsf(rel_angle));
    uint8_t dir   = (rel_angle >= 0) ? 0 : 1;

    Emm_Position(addr, dir, vel, acc, pulse, false, false);

    int32_t *pos_ptr = (addr == 1) ? &motor1_position : &motor2_position;
    *pos_ptr += (dir ? -pulse : pulse);
}

/**
 * @brief 云台控制入口
 * @param mode  0=绝对位置, 1=相对位置
 * @param addr  电机地址（1或2）
 * @param angle 角度值
 * @param vel   速度(RPM)
 * @param acc   加速度(0=直接启动)
 */
void GimbalControl(uint8_t mode, uint8_t addr, float angle, uint16_t vel, uint8_t acc)
{
    switch (mode)
    {
        case 0: AbsolutePositionMode(addr, angle, vel, acc); break;
        case 1: RelativePositionMode(addr, angle, vel, acc); break;
        default: break;
    }
}
