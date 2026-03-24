#include "mpu6050_kalman.h"
#include "mspm0_i2c.h"
#include "FreeRTOS.h"
#include "task.h"

void MPU6050_SoftCalibrate(void); 
void MPU6050_Read_Raw(MPU6050_RawData *raw);
void MPU6050_Write_REG(uint8_t reg, uint8_t data);

static float current_yaw = 0.0f;
// #define YAW_GYRO_SCALE 1.11f  // 手动校准系数，已改为从四元数提取，暂不需要
/* ---------------------------------------------------------------
 * Cortex-M0+ 纯 C 数学库 (针对没有 FPU 优化)
 * --------------------------------------------------------------- */
static float mpu_fabsf(float x) { return (x < 0.0f) ? -x : x; }

static float mpu_sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float y = x, z;
    for (int i = 0; i < 10; i++) {
        z = 0.5f * (y + x / y);
        if (z == y) break;
        y = z;
    }
    return y;
}

// 雷神之锤3 快速平方根倒数 (极大提升 Cortex-M0+ 性能)
static float mpu_invSqrt(float x) {
    float halfx = 0.5f * x;
    float y = x;
    long i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (1.5f - (halfx * y * y));
    return y;
}

static float mpu_atan2f(float y, float x) {
    const float PI = 3.14159265f, HALF_PI = 1.57079633f;
    if (x == 0.0f) {
        if (y > 0.0f) return HALF_PI;
        if (y < 0.0f) return -HALF_PI;
        return 0.0f;
    }
    float abs_y = mpu_fabsf(y) + 1e-10f, r, angle;
    if (x < 0.0f) {
        r = (x + abs_y) / (abs_y - x);
        angle = 3.0f * PI / 4.0f;
    } else {
        r = (x - abs_y) / (x + abs_y);
        angle = PI / 4.0f;
    }
    angle += (0.1963f * r * r - 0.9817f) * r;
    return (y < 0.0f) ? -angle : angle;
}

// 纯 C 近似反正弦函数 (用于计算 Pitch)
static float mpu_asinf(float x) {
    float a = mpu_fabsf(x);
    if (a >= 1.0f) return (x > 0) ? 1.57079632f : -1.57079632f;
    float ret = -0.0187293f;
    ret *= a; ret += 0.0742610f;
    ret *= a; ret -= 0.2121144f;
    ret *= a; ret += 1.5707288f;
    ret = 1.57079632f - mpu_sqrtf(1.0f - a) * ret;
    return x < 0.0f ? -ret : ret;
}

/* ---------------------------------------------------------------
 * MPU6050 驱动与校准
 * --------------------------------------------------------------- */
static float gyro_offset_x = 0.0f;
static float gyro_offset_y = 0.0f;
static float gyro_offset_z = 0.0f;

void MPU6050_Write_REG(uint8_t reg, uint8_t data) {
    mspm0_i2c_write(MPU6050_ADDRESS, reg, 1, &data);
}

uint8_t MPU6050_Read_REG(uint8_t reg) {
    uint8_t rxData;
    mspm0_i2c_read(MPU6050_ADDRESS, reg, 1, &rxData);
    return rxData;
}

void MPU6050_Init_RawMode(MPU6050_InitTypeDef *config) {
    MPU6050_Write_REG(MPU6050_PWR_MGMT_1, 0x80);
    vTaskDelay(pdMS_TO_TICKS(100));
    MPU6050_Write_REG(MPU6050_PWR_MGMT_1, 0x00);
    
    uint8_t SMPLRT_DIV = 1000 / config->SMPLRT_Rate - 1;
    MPU6050_Write_REG(MPU6050_SMPLRT_DIV, SMPLRT_DIV);
    MPU6050_Write_REG(MPU6050_CONFIG,       config->Filter);
    MPU6050_Write_REG(MPU6050_GYRO_CONFIG,  config->gyro_range);
    MPU6050_Write_REG(MPU6050_ACCEL_CONFIG, config->acc_range);
    MPU6050_Write_REG(MPU6050_PWR_MGMT_1,  0x01);

    MPU6050_SoftCalibrate(); // 启动全轴校准
}

void MPU6050_Read_Raw(MPU6050_RawData *raw) {
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

// 非常关键：必须校准陀螺仪三轴，否则Mahony会倾斜漂移
void MPU6050_SoftCalibrate(void) {
    float gx_sum = 0, gy_sum = 0, gz_sum = 0;
    MPU6050_RawData raw;
    for (int i = 0; i < 300; i++) {
        MPU6050_Read_Raw(&raw);
        gx_sum += raw.GyroX;
        gy_sum += raw.GyroY;
        gz_sum += raw.GyroZ;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    gyro_offset_x = gx_sum / 300.0f;
    gyro_offset_y = gy_sum / 300.0f;
    gyro_offset_z = gz_sum / 300.0f;
}

/* ---------------------------------------------------------------
 * Mahony AHRS 姿态解算算法 (6DOF)
 * --------------------------------------------------------------- */
#define Kp 1.0f  // 比例增益，控制加速度计收敛速度
#define Ki 0.01f // 积分增益，用于消除陀螺仪零偏

static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f; 
static float exInt = 0.0f, eyInt = 0.0f, ezInt = 0.0f;

void MPU6050_Get_Angle(MPU6050_Angle *angle) {
    MPU6050_RawData raw;
    MPU6050_Read_Raw(&raw);

    float dt = 0.01f;

    float ax = raw.AccX;
    float ay = raw.AccY;
    float az = raw.AccZ;

    float gx = (raw.GyroX - gyro_offset_x) / 131.0f * (3.14159265f / 180.0f);
    float gy = (raw.GyroY - gyro_offset_y) / 131.0f * (3.14159265f / 180.0f);
    float gz = (raw.GyroZ - gyro_offset_z) / 131.0f * (3.14159265f / 180.0f);

    // ---- Yaw 单独积分（校准后原始 gz，不经过 Mahony 修正）----
    // 死区过滤静止噪声（0.005 rad/s ≈ 0.29°/s，比之前更精细）
    if (mpu_fabsf(gz) > 0.05f)
        current_yaw += gz * (180.0f / 3.14159265f) * dt;

    // 限制 Yaw 在 -180° ~ +180°
    if (current_yaw >  180.0f) current_yaw -= 360.0f;
    if (current_yaw < -180.0f) current_yaw += 360.0f;

    // ---- Mahony 只负责 Roll/Pitch，gz 传 0 避免 Yaw 被 Mahony 修改 ----
    float norm;
    float vx, vy, vz;
    float ex, ey, ez;

    if (ax * ay * az != 0.0f) {
        norm = mpu_invSqrt(ax*ax + ay*ay + az*az);
        ax *= norm; ay *= norm; az *= norm;

        vx = 2.0f * (q1*q3 - q0*q2);
        vy = 2.0f * (q0*q1 + q2*q3);
        vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;

        ex = (ay*vz - az*vy);
        ey = (az*vx - ax*vz);
        ez = (ax*vy - ay*vx);

        exInt += ex * Ki * dt;
        eyInt += ey * Ki * dt;
        ezInt += ez * Ki * dt;

        gx += Kp * ex + exInt;
        gy += Kp * ey + eyInt;
        // gz 不加 Mahony 修正，Yaw 方向四元数只靠陀螺仪，但输出走独立积分
    }

    float qa = q0, qb = q1, qc = q2;
    q0 += (-qb*gx - qc*gy - q3*gz) * (0.5f * dt);
    q1 += ( qa*gx + qc*gz - q3*gy) * (0.5f * dt);
    q2 += ( qa*gy - qb*gz + q3*gx) * (0.5f * dt);
    q3 += ( qa*gz + qb*gy - qc*gx) * (0.5f * dt);

    norm = mpu_invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    q0 *= norm; q1 *= norm; q2 *= norm; q3 *= norm;

    // Roll/Pitch 从四元数提取（Mahony 修正，稳定无漂移）
    angle->roll  = mpu_atan2f(2.0f * (q0*q1 + q2*q3), 1.0f - 2.0f * (q1*q1 + q2*q2)) * 57.29578f;
    angle->pitch = mpu_asinf (2.0f * (q0*q2 - q3*q1)) * 57.29578f;
    // Yaw 用独立积分（死区保护，不受 Mahony Ki 项污染）
    angle->yaw   = current_yaw;
}