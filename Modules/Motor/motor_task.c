#include "motor_task.h"
#include "motor_driver.h"
#include "encoder.h"
#include "tracking_task.h"
#include "button.h"
#include "task.h"
#include "queue.h"

QueueHandle_t xMotorSpeedQueue = NULL;
Speed_PID_Controller gMotorLeftPID;
Speed_PID_Controller gMotorRightPID;

float g_base_speed = 0.5f;  // 基础目标速度（m/s），可运行时修改

// 软启动：每个 10ms 周期最多允许 duty 增加的幅度
#define RAMP_STEP  20.0f

static float ramp_duty(float current, float target)
{
    float diff = target - current;
    if (diff >  RAMP_STEP) return current + RAMP_STEP;
    if (diff < -RAMP_STEP) return current - RAMP_STEP;
    return target;
}

// 丢线状态机
typedef enum {
    MOTOR_STATE_IDLE,           // 等待按键
    MOTOR_STATE_TRACKING,       // 正常循迹
    MOTOR_STATE_LOST_STRAIGHT,  // 丢线后直行 0.3 秒
    MOTOR_STATE_LOST_TURN,      // 直行结束后原地左转（右轮转左轮停）
    MOTOR_STATE_STOPPED,        // 完成指定圈数，停止
} MotorState_t;

void motorTask(void *pvParameters)
{
    (void)pvParameters;

    TrackResult track_result;
    MotorState_t state = MOTOR_STATE_IDLE;
    uint8_t lost_count    = 0;   // 累计丢线次数
    uint8_t lost_limit    = 4;   // 触发停止的丢线次数（1圈=4，2圈=8）
    uint32_t state_ticks  = 0;   // 进入当前状态时的 tick
    const float dt = 0.01f;

    // 软启动当前输出值
    float out_l = 0.0f;
    float out_r = 0.0f;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    while (1)
    {
        Encoder_UpdateSpeed();

        // 读最新循迹数据（非阻塞）
        bool track_valid = (xQueuePeek(xTrackResultQueue, &track_result, 0) == pdTRUE);
        bool line_found  = track_valid && (track_result.state != TRACK_LOST);

        float duty_l = 0.0f;
        float duty_r = 0.0f;

        switch (state)
        {
            case MOTOR_STATE_IDLE:
            {
                // 等待按键：A26 = 1圈，A25 = 2圈
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;
                if (Button_A26_Pressed()) {
                    lost_count = 0;
                    lost_limit = 4;
                    state = MOTOR_STATE_TRACKING;
                } else if (Button_A25_Pressed()) {
                    lost_count = 0;
                    lost_limit = 8;
                    state = MOTOR_STATE_TRACKING;
                }
                goto skip_output;
            }

            case MOTOR_STATE_TRACKING:
            {
                if (!line_found) {
                    // 刚丢线
                    lost_count++;
                    if (lost_count >= lost_limit) {
                        state = MOTOR_STATE_STOPPED;
                    } else {
                        state = MOTOR_STATE_LOST_STRAIGHT;
                        state_ticks = xTaskGetTickCount();
                    }
                    break;
                }

                // 正常循迹：转向 PID 叠加基础速度
                float steering_output = Steering_PID_Compute(
                    &gSteeringPID, 0.0f, track_result.track_error, dt);

                float target_l = g_base_speed + steering_output;
                float target_r = g_base_speed - steering_output;

                float pid_l = Speed_PID_Compute(&gMotorLeftPID,  target_l, g_encoder_left.speed_mps,  dt);
                float pid_r = Speed_PID_Compute(&gMotorRightPID, target_r, g_encoder_right.speed_mps, dt);

                // 软启动限速
                duty_l = ramp_duty(out_l, pid_l);
                duty_r = ramp_duty(out_r, pid_r);
                break;
            }

            case MOTOR_STATE_LOST_STRAIGHT:
            {
                // 直行 2.5 秒
                uint32_t elapsed = xTaskGetTickCount() - state_ticks;
                if (elapsed >= pdMS_TO_TICKS(300)) {
                    state = MOTOR_STATE_LOST_TURN;
                    state_ticks = xTaskGetTickCount();
                    break;
                }
                float pid_l = Speed_PID_Compute(&gMotorLeftPID,  g_base_speed, g_encoder_left.speed_mps,  dt);
                float pid_r = Speed_PID_Compute(&gMotorRightPID, g_base_speed, g_encoder_right.speed_mps, dt);

                duty_l = ramp_duty(out_l, pid_l);
                duty_r = ramp_duty(out_r, pid_r);
                break;
            }

            case MOTOR_STATE_LOST_TURN:
            {
                if (line_found) {
                    // 找到线，恢复循迹
                    state = MOTOR_STATE_TRACKING;
                    break;
                }
                // 原地左转：右轮正转，左轮停
                out_l = 0.0f;
                Motor_SetPWM(0, 0.0f);

                float pid_r = Speed_PID_Compute(&gMotorRightPID, g_base_speed, g_encoder_right.speed_mps, dt);
                out_r = ramp_duty(out_r, pid_r);
                Motor_SetPWM(1, out_r);

                goto skip_output;
            }

            case MOTOR_STATE_STOPPED:
            default:
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;
                // 等待再次按键重新出发
                state = MOTOR_STATE_IDLE;
                goto skip_output;
        }

        out_l = duty_l;
        out_r = duty_r;
        Motor_SetPWM(0, duty_l);
        Motor_SetPWM(1, duty_r);

    skip_output:;

        MotorSpeed_t speeds = {
            .left_mps  = g_encoder_left.speed_mps,
            .right_mps = g_encoder_right.speed_mps,
        };
        xQueueOverwrite(xMotorSpeedQueue, &speeds);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void Motor_TaskInit(void)
{
    Encoder_Init();
    Motor_DriverInit();

    Speed_PID_Init(&gMotorLeftPID,  1500.0f, 0.0f, 0.0f, 800.0f, 0.6f);
    Speed_PID_Init(&gMotorRightPID, 1500.0f, 0.0f, 0.0f, 800.0f, 0.6f);

    xMotorSpeedQueue = xQueueCreate(1, sizeof(MotorSpeed_t));

    xTaskCreate(motorTask, "motorTask", 0x200, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
