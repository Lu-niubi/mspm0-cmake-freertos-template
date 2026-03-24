#ifndef MPU6050_KALMAN_H
#define MPU6050_KALMAN_H

#include <stdint.h>

// MPU6050 I2C地址
#define MPU6050_ADDRESS 0x68

// 寄存器定义
#define MPU6050_SMPLRT_DIV       0x19
#define MPU6050_CONFIG           0x1A
#define MPU6050_GYRO_CONFIG      0x1B
#define MPU6050_ACCEL_CONFIG     0x1C
#define MPU6050_ACCEL_XOUT_H     0x3B
#define MPU6050_ACCEL_XOUT_L     0x3C
#define MPU6050_ACCEL_YOUT_H     0x3D
#define MPU6050_ACCEL_YOUT_L     0x3E
#define MPU6050_ACCEL_ZOUT_H     0x3F
#define MPU6050_ACCEL_ZOUT_L     0x40
#define MPU6050_TEMP_OUT_H       0x41
#define MPU6050_TEMP_OUT_L       0x42
#define MPU6050_GYRO_XOUT_H      0x43
#define MPU6050_GYRO_XOUT_L      0x44
#define MPU6050_GYRO_YOUT_H      0x45
#define MPU6050_GYRO_YOUT_L      0x46
#define MPU6050_GYRO_ZOUT_H      0x47
#define MPU6050_GYRO_ZOUT_L      0x48
#define MPU6050_PWR_MGMT_1       0x6B
#define MPU6050_WHO_AM_I         0x75

// 滤波器带宽
typedef enum {
    Band_256Hz = 0x00,
    Band_186Hz,
    Band_96Hz,
    Band_43Hz,
    Band_21Hz,
    Band_10Hz,
    Band_5Hz
} Filter_Typedef;

// 陀螺仪量程
typedef enum {
    gyro_250  = 0x00,
    gyro_500  = 0x08,
    gyro_1000 = 0x10,
    gyro_2000 = 0x18
} GYRO_CONFIG_Typedef;

// 加速度计量程
typedef enum {
    acc_2g  = 0x00,
    acc_4g  = 0x08,
    acc_8g  = 0x10,
    acc_16g = 0x18
} ACCEL_CONFIG_Typedef;

// 原始数据结构
typedef struct {
    int16_t AccX;
    int16_t AccY;
    int16_t AccZ;
    int16_t GyroX;
    int16_t GyroY;
    int16_t GyroZ;
    int16_t Temp;
} MPU6050_RawData;

// 角度数据结构
typedef struct {
    float roll;
    float pitch;
    float yaw;
} MPU6050_Angle;

// 初始化配置结构
typedef struct {
    uint16_t SMPLRT_Rate;           // 采样率(Hz)
    Filter_Typedef Filter;          // 滤波器带宽
    GYRO_CONFIG_Typedef gyro_range; // 陀螺仪量程
    ACCEL_CONFIG_Typedef acc_range; // 加速度计量程
} MPU6050_InitTypeDef;

// 卡尔曼滤波器结构
typedef struct {
    float q; // 过程噪声协方差
    float r; // 测量噪声协方差
    float x; // 状态估计值
    float p; // 估计误差协方差
    float k; // 卡尔曼增益
} MPU6050_KalmanFilter;

// 函数声明
void MPU6050_Init_RawMode(MPU6050_InitTypeDef *config);
void MPU6050_Read_Raw(MPU6050_RawData *raw);
void MPU6050_Get_Angle(MPU6050_Angle *angle);
float KalmanFilter_Update(MPU6050_KalmanFilter *kf, float measurement);
void MPU6050_SoftCalibrate_Z(void);
uint8_t MPU6050_Read_REG(uint8_t reg);
void MPU6050_Write_REG(uint8_t reg, uint8_t data);

#endif // MPU6050_KALMAN_H
