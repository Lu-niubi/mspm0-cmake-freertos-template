#ifndef __STEP_MOTOR_CMD_H
#define __STEP_MOTOR_CMD_H

#include "stdbool.h"
#include "math.h"
#include "ti_msp_dl_config.h"
#include <stdint.h>

#define SM_Speed          10
#define SM_Acceleration   0
#define SM_Direction_CW   99
#define SM_Direction_CCW  0
#define SM_Position_Mood  1

#define Step_Motor_1  1
#define Step_Motor_2  2

void Emm_Position(uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc,
                  uint32_t clk, bool raF, bool snF);
void RelativePositionMode(uint8_t addr, float rel_angle, uint16_t vel, uint8_t acc);
void AbsolutePositionMode(uint8_t addr, float target_angle, uint16_t vel, uint8_t acc);
void GimbalControl(uint8_t mode, uint8_t addr, float angle, uint16_t vel, uint8_t acc);

#endif /* __STEP_MOTOR_CMD_H */
