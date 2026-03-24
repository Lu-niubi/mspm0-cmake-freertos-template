#ifndef __PID_H_
#define __PID_H_

#define PI 3.1415926

typedef struct {
    float Kp;           // 比例系数
    float Ki;           // 积分系数
    float Kd;           // 微分系数
    float integral;     // 积分项
    float prev_error;   // 上一次误差
    float output_limit; // 输出限幅
    float filter_alpha;  //输出滤波
    float filtered_output; //滤波后的值
} Speed_PID_Controller;

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float integral;
    float prev_error;
    float filtered_output;
    float filter_alpha;
    float output_limit;
} Steering_PID_Controller;

extern float e;

// PID初始化函数
void Speed_PID_Init(Speed_PID_Controller *pid, float Kp, float Ki, float Kd, float output_limit, float filter_output);

// PID计算函数
float Speed_PID_Compute(Speed_PID_Controller *pid, float target, float actual, float dt);
void Speed_PID_SetParams(Speed_PID_Controller* pid, float kp, float ki, float kd);

void Steering_PID_Init(Steering_PID_Controller* pid, float Kp, float Ki, float Kd, float output_limit, float filter_alpha);
float Steering_PID_Compute(Steering_PID_Controller* pid, float target, float actual, float dt);

#endif
