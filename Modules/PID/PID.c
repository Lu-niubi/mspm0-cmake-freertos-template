#include "PID.h"

float e;

/**
 * @brief 初始化速度环PID控制器
 */
void Speed_PID_Init(Speed_PID_Controller *pid, float Kp, float Ki, float Kd, float output_limit, float filter_output) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->filter_alpha = filter_output;
    pid->output_limit = output_limit;
}

/**
 * @brief 计算PID输出值
 */
float Speed_PID_Compute(Speed_PID_Controller *pid, float target, float actual, float dt) {
    // 计算误差
    float error = target - actual;
    e = error;
    // 比例项
    float P = pid->Kp * error;

    // 积分项（带积分限幅）
    pid->integral += error * dt;

    // 积分抗饱和 - 限制积分项范围
    if (pid->integral > pid->output_limit) {
        pid->integral = pid->output_limit;
    } else if (pid->integral < -pid->output_limit) {
        pid->integral = -pid->output_limit;
    }

    float I = pid->Ki * pid->integral;

    // 微分项（使用误差的微分）
    float derivative = (error - pid->prev_error) / dt;
    float D = pid->Kd * derivative;

    float raw_output = P + I + D;

    // 输出限幅
    if (raw_output > pid->output_limit) {
        raw_output = pid->output_limit;
    } else if (raw_output < -pid->output_limit) {
        raw_output = -pid->output_limit;
    }

    // 应用一阶低通滤波
    pid->filtered_output = pid->filter_alpha * pid->filtered_output +
                           (1.0f - pid->filter_alpha) * raw_output;

    pid->prev_error = error;
    return pid->filtered_output;
}

/**
 * @brief 初始化转向PID控制器
 */
void Steering_PID_Init(Steering_PID_Controller* pid, float Kp, float Ki, float Kd,
                       float output_limit, float filter_alpha) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->filter_alpha = filter_alpha;
    pid->output_limit = output_limit;
}

/**
 * @brief 计算转向PID输出值
 */
float Steering_PID_Compute(Steering_PID_Controller* pid, float target, float actual, float dt) {
    float error = target - actual;

    // 比例项
    float P = pid->Kp * error;

    // 积分项
    pid->integral += error * dt;

    // 积分抗饱和
    if (pid->integral > pid->output_limit) {
        pid->integral = pid->output_limit;
    } else if (pid->integral < -pid->output_limit) {
        pid->integral = -pid->output_limit;
    }

    float I = pid->Ki * pid->integral;

    // 微分项
    float derivative = (error - pid->prev_error) / dt;
    float D = pid->Kd * derivative;

    // 计算原始输出
    float raw_output = P + I + D;

    // 输出限幅
    if (raw_output > pid->output_limit) {
        raw_output = pid->output_limit;
    } else if (raw_output < -pid->output_limit) {
        raw_output = -pid->output_limit;
    }

    // 低通滤波
    pid->filtered_output = pid->filter_alpha * pid->filtered_output +
                           (1.0f - pid->filter_alpha) * raw_output;

    // 更新上一误差
    pid->prev_error = error;

    return pid->filtered_output;
}
