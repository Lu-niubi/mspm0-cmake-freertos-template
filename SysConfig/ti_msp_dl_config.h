/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the LP_MSPM0G3507
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_LP_MSPM0G3507
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMA1
#define PWM_0_INST_IRQHandler                                   TIMA1_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMA1_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                             10000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOB
#define GPIO_PWM_0_C0_PIN                                          DL_GPIO_PIN_4
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM17)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM17_PF_TIMA1_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOB
#define GPIO_PWM_0_C1_PIN                                          DL_GPIO_PIN_1
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM13)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM13_PF_TIMA1_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX




/* Defines for I2C_MPU6050 */
#define I2C_MPU6050_INST                                                    I2C0
#define I2C_MPU6050_INST_IRQHandler                              I2C0_IRQHandler
#define I2C_MPU6050_INST_INT_IRQN                                  I2C0_INT_IRQn
#define I2C_MPU6050_BUS_SPEED_HZ                                          400000
#define GPIO_I2C_MPU6050_SDA_PORT                                          GPIOA
#define GPIO_I2C_MPU6050_SDA_PIN                                  DL_GPIO_PIN_10
#define GPIO_I2C_MPU6050_IOMUX_SDA                               (IOMUX_PINCM21)
#define GPIO_I2C_MPU6050_IOMUX_SDA_FUNC                IOMUX_PINCM21_PF_I2C0_SDA
#define GPIO_I2C_MPU6050_SCL_PORT                                          GPIOA
#define GPIO_I2C_MPU6050_SCL_PIN                                  DL_GPIO_PIN_11
#define GPIO_I2C_MPU6050_IOMUX_SCL                               (IOMUX_PINCM22)
#define GPIO_I2C_MPU6050_IOMUX_SCL_FUNC                IOMUX_PINCM22_PF_I2C0_SCL


/* Defines for UART0 */
#define UART0_INST                                                         UART0
#define UART0_INST_FREQUENCY                                            40000000
#define UART0_INST_IRQHandler                                   UART0_IRQHandler
#define UART0_INST_INT_IRQN                                       UART0_INT_IRQn
#define GPIO_UART0_RX_PORT                                                 GPIOA
#define GPIO_UART0_TX_PORT                                                 GPIOA
#define GPIO_UART0_RX_PIN                                          DL_GPIO_PIN_1
#define GPIO_UART0_TX_PIN                                          DL_GPIO_PIN_0
#define GPIO_UART0_IOMUX_RX                                       (IOMUX_PINCM2)
#define GPIO_UART0_IOMUX_TX                                       (IOMUX_PINCM1)
#define GPIO_UART0_IOMUX_RX_FUNC                        IOMUX_PINCM2_PF_UART0_RX
#define GPIO_UART0_IOMUX_TX_FUNC                        IOMUX_PINCM1_PF_UART0_TX
#define UART0_BAUD_RATE                                                 (115200)
#define UART0_IBRD_40_MHZ_115200_BAUD                                       (21)
#define UART0_FBRD_40_MHZ_115200_BAUD                                       (45)





/* Port definition for Pin Group PORTA */
#define PORTA_PORT                                                       (GPIOA)

/* Defines for LED_USER: GPIOA.4 with pinCMx 9 on package pin 44 */
#define PORTA_LED_USER_PIN                                       (DL_GPIO_PIN_4)
#define PORTA_LED_USER_IOMUX                                      (IOMUX_PINCM9)
/* Port definition for Pin Group GPIO_LED */
#define GPIO_LED_PORT                                                    (GPIOB)

/* Defines for PIN_LED: GPIOB.22 with pinCMx 50 on package pin 21 */
#define GPIO_LED_PIN_LED_PIN                                    (DL_GPIO_PIN_22)
#define GPIO_LED_PIN_LED_IOMUX                                   (IOMUX_PINCM50)
/* Defines for L1: GPIOA.24 with pinCMx 54 on package pin 25 */
#define Tracking_L1_PORT                                                 (GPIOA)
#define Tracking_L1_PIN                                         (DL_GPIO_PIN_24)
#define Tracking_L1_IOMUX                                        (IOMUX_PINCM54)
/* Defines for L2: GPIOB.24 with pinCMx 52 on package pin 23 */
#define Tracking_L2_PORT                                                 (GPIOB)
#define Tracking_L2_PIN                                         (DL_GPIO_PIN_24)
#define Tracking_L2_IOMUX                                        (IOMUX_PINCM52)
/* Defines for L3: GPIOB.25 with pinCMx 56 on package pin 27 */
#define Tracking_L3_PORT                                                 (GPIOB)
#define Tracking_L3_PIN                                         (DL_GPIO_PIN_25)
#define Tracking_L3_IOMUX                                        (IOMUX_PINCM56)
/* Defines for L4: GPIOB.20 with pinCMx 48 on package pin 19 */
#define Tracking_L4_PORT                                                 (GPIOB)
#define Tracking_L4_PIN                                         (DL_GPIO_PIN_20)
#define Tracking_L4_IOMUX                                        (IOMUX_PINCM48)
/* Defines for R4: GPIOA.14 with pinCMx 36 on package pin 7 */
#define Tracking_R4_PORT                                                 (GPIOA)
#define Tracking_R4_PIN                                         (DL_GPIO_PIN_14)
#define Tracking_R4_IOMUX                                        (IOMUX_PINCM36)
/* Defines for R3: GPIOB.18 with pinCMx 44 on package pin 15 */
#define Tracking_R3_PORT                                                 (GPIOB)
#define Tracking_R3_PIN                                         (DL_GPIO_PIN_18)
#define Tracking_R3_IOMUX                                        (IOMUX_PINCM44)
/* Defines for R2: GPIOB.19 with pinCMx 45 on package pin 16 */
#define Tracking_R2_PORT                                                 (GPIOB)
#define Tracking_R2_PIN                                         (DL_GPIO_PIN_19)
#define Tracking_R2_IOMUX                                        (IOMUX_PINCM45)
/* Defines for R1: GPIOB.14 with pinCMx 31 on package pin 2 */
#define Tracking_R1_PORT                                                 (GPIOB)
#define Tracking_R1_PIN                                         (DL_GPIO_PIN_14)
#define Tracking_R1_IOMUX                                        (IOMUX_PINCM31)
/* Port definition for Pin Group GPIO_Encoder */
#define GPIO_Encoder_PORT                                                (GPIOA)

/* Defines for PIN_Front_Left_A: GPIOA.15 with pinCMx 37 on package pin 8 */
// pins affected by this interrupt request:["PIN_Front_Left_A","PIN_Front_Right_A"]
#define GPIO_Encoder_INT_IRQN                                   (GPIOA_INT_IRQn)
#define GPIO_Encoder_INT_IIDX                   (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define GPIO_Encoder_PIN_Front_Left_A_IIDX                  (DL_GPIO_IIDX_DIO15)
#define GPIO_Encoder_PIN_Front_Left_A_PIN                       (DL_GPIO_PIN_15)
#define GPIO_Encoder_PIN_Front_Left_A_IOMUX                      (IOMUX_PINCM37)
/* Defines for PIN_Front_Left_B: GPIOA.16 with pinCMx 38 on package pin 9 */
#define GPIO_Encoder_PIN_Front_Left_B_PIN                       (DL_GPIO_PIN_16)
#define GPIO_Encoder_PIN_Front_Left_B_IOMUX                      (IOMUX_PINCM38)
/* Defines for PIN_Front_Right_A: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GPIO_Encoder_PIN_Front_Right_A_IIDX                 (DL_GPIO_IIDX_DIO22)
#define GPIO_Encoder_PIN_Front_Right_A_PIN                      (DL_GPIO_PIN_22)
#define GPIO_Encoder_PIN_Front_Right_A_IOMUX                     (IOMUX_PINCM47)
/* Defines for PIN_Front_Right_B: GPIOA.17 with pinCMx 39 on package pin 10 */
#define GPIO_Encoder_PIN_Front_Right_B_PIN                      (DL_GPIO_PIN_17)
#define GPIO_Encoder_PIN_Front_Right_B_IOMUX                     (IOMUX_PINCM39)
/* Defines for GPIO_AIN1: GPIOA.13 with pinCMx 35 on package pin 6 */
#define GPIO_IN_GPIO_AIN1_PORT                                           (GPIOA)
#define GPIO_IN_GPIO_AIN1_PIN                                   (DL_GPIO_PIN_13)
#define GPIO_IN_GPIO_AIN1_IOMUX                                  (IOMUX_PINCM35)
/* Defines for GPIO_AIN2: GPIOA.12 with pinCMx 34 on package pin 5 */
#define GPIO_IN_GPIO_AIN2_PORT                                           (GPIOA)
#define GPIO_IN_GPIO_AIN2_PIN                                   (DL_GPIO_PIN_12)
#define GPIO_IN_GPIO_AIN2_IOMUX                                  (IOMUX_PINCM34)
/* Defines for GPIO_BIN1: GPIOB.0 with pinCMx 12 on package pin 47 */
#define GPIO_IN_GPIO_BIN1_PORT                                           (GPIOB)
#define GPIO_IN_GPIO_BIN1_PIN                                    (DL_GPIO_PIN_0)
#define GPIO_IN_GPIO_BIN1_IOMUX                                  (IOMUX_PINCM12)
/* Defines for GPIO_BIN2: GPIOB.16 with pinCMx 33 on package pin 4 */
#define GPIO_IN_GPIO_BIN2_PORT                                           (GPIOB)
#define GPIO_IN_GPIO_BIN2_PIN                                   (DL_GPIO_PIN_16)
#define GPIO_IN_GPIO_BIN2_IOMUX                                  (IOMUX_PINCM33)





/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_SYSCTL_CLK_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_I2C_MPU6050_init(void);
void SYSCFG_DL_UART0_init(void);

void SYSCFG_DL_SYSTICK_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
