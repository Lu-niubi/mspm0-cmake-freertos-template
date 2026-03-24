#include "mpu6050_kalman.h"
#include "mspm0_i2c.h"
#include "FreeRTOS.h"
#include "task.h"

/* ---------------------------------------------------------------
 * Cortex-M0+ (thumb/v6-m/nofp) 的 libm.a 不含 sqrtf/atan2f，
 * 此处用纯 C 实现，精度足够 IMU 姿态估计使用。
 * --------------------------------------------------------------- */
static float mpu_fabsf(float x)
{
    return (x < 0.0f) ? -x : x;
}

static float mpu_sqrtf(float x)
{
    if (x <= 0.0f) return 0.0f;
    float y = x, z;
    int i;
    for (i = 0; i < 10; i++) {
        z = 0.5f * (y + x / y);
        if (z == y) break;
        y = z;
    }
    return y;
}

static float mpu_atan2f(float y, float x)
{
    const float PI      = 3.14159265f;
    const float HALF_PI = 1.57079633f;

    if (x == 0.0f) {
        if (y > 0.0f) return  HALF_PI;
        if (y < 0.0f) return -HALF_PI;
        return 0.0f;
    }

    float abs_y = mpu_fabsf(y) + 1e-10f;
    float r, angle;

    if (x < 0.0f) {
        r     = (x + abs_y) / (abs_y - x);
        angle = 3.0f * PI / 4.0f;
    } else {
        r     = (x - abs_y) / (x + abs_y);
        angle = PI / 4.0f;
    }
    angle += (0.1963f * r * r - 0.9817f) * r;

    return (y < 0.0f) ? -angle : angle;
}

/* --------------------------------------------------------------- */

static float gyro_zero_z = 0.0f;

void MPU6050_Write_REG(uint8_t reg, uint8_t data)
{
    mspm0_i2c_write(MPU6050_ADDRESS, reg, 1, &data);
}

uint8_t MPU6050_Read_REG(uint8_t reg)
{
    uint8_t rxData;
    mspm0_i2c_read(MPU6050_ADDRESS, reg, 1, &rxData);
    return rxData;
}

void MPU6050_Init_RawMode(MPU6050_InitTypeDef *config)
{
    uint8_t SMPLRT_DIV;

    MPU6050_Write_REG(MPU6050_PWR_MGMT_1, 0x80);
    vTaskDelay(pdMS_TO_TICKS(100));
    MPU6050_Write_REG(MPU6050_PWR_MGMT_1, 0x00);

    if (config->SMPLRT_Rate >= 1000) config->SMPLRT_Rate = 1000;
    else if (config->SMPLRT_Rate < 4)  config->SMPLRT_Rate = 4;
    SMPLRT_DIV = 1000 / config->SMPLRT_Rate - 1;
    MPU6050_Write_REG(MPU6050_SMPLRT_DIV, SMPLRT_DIV);

    MPU6050_Write_REG(MPU6050_CONFIG,       config->Filter);
    MPU6050_Write_REG(MPU6050_GYRO_CONFIG,  config->gyro_range);
    MPU6050_Write_REG(MPU6050_ACCEL_CONFIG, config->acc_range);
    MPU6050_Write_REG(MPU6050_PWR_MGMT_1,  0x01);

    MPU6050_SoftCalibrate_Z();
}

void MPU6050_Read_Raw(MPU6050_RawData *raw)
{
    uint8_t buffer[14];
    mspm0_i2c_read(MPU6050_ADDRESS, MPU6050_ACCEL_XOUT_H, 14, buffer);

    raw->AccX  = (int16_t)((buffer[0]  << 8) | buffer[1]);
    raw->AccY  = (int16_t)((buffer[2]  << 8) | buffer[3]);
    raw->AccZ  = (int16_t)((buffer[4]  << 8) | buffer[5]);
    raw->Temp  = (int16_t)((buffer[6]  << 8) | buffer[7]);
    raw->GyroX = (int16_t)((buffer[8]  << 8) | buffer[9]);
    raw->GyroY = (int16_t)((buffer[10] << 8) | buffer[11]);
    raw->GyroZ = (int16_t)((buffer[12] << 8) | buffer[13]);
}

void MPU6050_SoftCalibrate_Z(void)
{
    uint16_t i;
    float gz_sum = 0.0f;
    MPU6050_RawData raw;

    for (i = 0; i < 200; i++) {
        MPU6050_Read_Raw(&raw);
        gz_sum += (float)raw.GyroZ;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    gyro_zero_z = gz_sum / 200.0f;
}

float KalmanFilter_Update(MPU6050_KalmanFilter *kf, float measurement)
{
    kf->p = kf->p + kf->q;
    kf->k = kf->p / (kf->p + kf->r);
    kf->x = kf->x + kf->k * (measurement - kf->x);
    kf->p = (1.0f - kf->k) * kf->p;
    return kf->x;
}

void MPU6050_Get_Angle(MPU6050_Angle *angle)
{
    static MPU6050_KalmanFilter kf_roll  = {.q=0.001f, .r=0.1f, .x=0, .p=1};
    static MPU6050_KalmanFilter kf_pitch = {.q=0.001f, .r=0.1f, .x=0, .p=1};
    static float Gyroscope_roll  = 0.0f;
    static float Gyroscope_pitch = 0.0f;
    static float yaw = 0.0f;

    const float dt = 0.01f;

    MPU6050_RawData raw;
    MPU6050_Read_Raw(&raw);

    float Ax = raw.AccX / 16384.0f;
    float Ay = raw.AccY / 16384.0f;
    float Az = raw.AccZ / 16384.0f;

    float Gx = raw.GyroX / 131.0f * (3.1415926f / 180.0f) * dt;
    float Gy = raw.GyroY / 131.0f * (3.1415926f / 180.0f) * dt;
    float Gz = (raw.GyroZ - gyro_zero_z) / 131.0f * (3.1415926f / 180.0f) * dt;

    float absAcc = mpu_sqrtf(Ax*Ax + Ay*Ay + Az*Az);
    float weight = (absAcc > 1.2f) ? 0.8f : 1.0f;

    float acc_roll  =  mpu_atan2f(Ay, Az) * 180.0f / 3.1415926f;
    float acc_pitch = -mpu_atan2f(Ax, Az) * 180.0f / 3.1415926f;

    Gyroscope_roll  += Gy * (180.0f / 3.1415926f);
    Gyroscope_pitch += Gx * (180.0f / 3.1415926f);

    float raw_roll  = weight * acc_roll  + (1.0f - weight) * Gyroscope_roll;
    float raw_pitch = weight * acc_pitch + (1.0f - weight) * Gyroscope_pitch;

    angle->roll  = KalmanFilter_Update(&kf_roll,  raw_roll);
    angle->pitch = KalmanFilter_Update(&kf_pitch, raw_pitch);

    if (mpu_fabsf(Gz) >= 0.0010f)//0.0010
        yaw += Gz * (180.0f / 3.1415926f);
    angle->yaw = yaw;
}
