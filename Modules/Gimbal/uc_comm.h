#ifndef __UC_COMM_H__
#define __UC_COMM_H__

#include <stdint.h>

/*
 * 上位机通信协议（UART_2_INST = UART3，PB12/PB13，9600baud）
 *
 * 帧格式（6字节定长）：
 *   [0xFE] [yaw_hi] [yaw_lo] [pitch_hi] [pitch_lo] [0xFF]
 *
 *   yaw / pitch：int16_t 大端，单位 0.1°
 *   实际角度 = int16_t值 / 10.0f
 *
 * 串口助手测试示例（HEX发送）：
 *   yaw +90.0°   → FE 03 84 00 00 FF
 *   yaw -90.0°   → FE FC 7C 00 00 FF
 *   yaw +5.0°    → FE 00 32 00 00 FF
 *   pitch +10.0° → FE 00 00 00 64 FF
 */

/**
 * @brief 初始化并使能 UART_2(UART3) RX 中断
 *        必须在 Gimbal_TaskInit() 中调用，xGimbalCmdQueue 已创建后再调用
 */
void UC_Comm_Init(void);

#endif /* __UC_COMM_H__ */
