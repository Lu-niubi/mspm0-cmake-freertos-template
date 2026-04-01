#include "motor_task.h"
#include "motor_driver.h"
#include "encoder.h"
#include "tracking_task.h"
#include "button.h"
#include "oled_software_i2c.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

QueueHandle_t xMotorSpeedQueue = NULL;
Speed_PID_Controller gMotorLeftPID;
Speed_PID_Controller gMotorRightPID;

float g_base_speed = 0.5f;

#define RAMP_STEP  20.0f

static float ramp_duty(float current, float target)
{
    float diff = target - current;
    if (diff >  RAMP_STEP) return current + RAMP_STEP;
    if (diff < -RAMP_STEP) return current - RAMP_STEP;
    return target;
}

/* 刷新 OLED 选圈界面 */
static void oled_show_select(uint8_t laps)
{
    char buf[16];
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t *)"Select Laps:", 16);
    snprintf(buf, sizeof(buf), "  %d  lap(s)", (int)laps);
    OLED_ShowString(0, 2, (uint8_t *)buf, 16);
    OLED_ShowString(0, 4, (uint8_t *)"A26+ A25- B26OK", 16);
}

/* 刷新 OLED 倒计时界面 */
static void oled_show_countdown(uint8_t laps, uint8_t sec)
{
    char buf[16];
    OLED_ShowString(0, 0, (uint8_t *)"Ready! Start in:", 16);
    snprintf(buf, sizeof(buf), "  %d lap(s)  %ds", (int)laps, (int)sec);
    OLED_ShowString(0, 2, (uint8_t *)buf, 16);
    OLED_ShowString(0, 4, (uint8_t *)"                ", 16);
}

/* 刷新 OLED 运行界面（显示当前圈数进度） */
static void oled_show_running(uint8_t laps, uint8_t lost_count)
{
    char buf[16];
    uint8_t corners = lost_count;         // 每次丢线 = 过一个角
    uint8_t total   = laps * 4;
    snprintf(buf, sizeof(buf), "Corner %d/%d", (int)corners, (int)total);
    OLED_ShowString(0, 0, (uint8_t *)"Running...      ", 16);
    OLED_ShowString(0, 2, (uint8_t *)buf, 16);
}

typedef enum {
    MOTOR_STATE_IDLE,
    MOTOR_STATE_COUNTDOWN,
    MOTOR_STATE_TRACKING,
    MOTOR_STATE_LOST_STRAIGHT,
    MOTOR_STATE_LOST_TURN,
    MOTOR_STATE_STOPPED,
} MotorState_t;

void motorTask(void *pvParameters)
{
    (void)pvParameters;

    /* 在任务上下文中初始化 OLED（内部用 vTaskDelay，必须在任务内） */
    OLED_Init();

    /* 等 OLED 初始化完成（main 里在任务启动前调用） */

    TrackResult track_result;
    MotorState_t state     = MOTOR_STATE_IDLE;
    uint8_t lost_count     = 0;
    uint8_t lost_limit     = 4;    /* 1圈=4 */
    uint8_t selected_laps  = 1;    /* 默认1圈 */
    uint32_t state_ticks   = 0;
    const float dt = 0.01f;

    float out_l = 0.0f;
    float out_r = 0.0f;

    /* 先显示选圈界面 */
    oled_show_select(selected_laps);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); /* 20ms 周期兼做消抖 */

    while (1)
    {
        ButtonEvent_t btn = Button_Scan();

        Encoder_UpdateSpeed();

        bool track_valid = (xQueuePeek(xTrackResultQueue, &track_result, 0) == pdTRUE);
        bool line_found  = track_valid && (track_result.state != TRACK_LOST);

        float duty_l = 0.0f;
        float duty_r = 0.0f;

        switch (state)
        {
            /* ---- 选圈界面 ---- */
            case MOTOR_STATE_IDLE:
            {
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;

                bool need_refresh = false;
                if (btn.a26_edge) {
                    selected_laps++;
                    if (selected_laps > 9) selected_laps = 1;
                    need_refresh = true;
                }
                if (btn.a25_edge) {
                    if (selected_laps > 1) selected_laps--;
                    need_refresh = true;
                }
                if (need_refresh) {
                    oled_show_select(selected_laps);
                }

                if (btn.b26_edge) {
                    /* 确认圈数，进入倒计时 */
                    lost_limit  = selected_laps * 4;
                    lost_count  = 0;
                    state       = MOTOR_STATE_COUNTDOWN;
                    state_ticks = xTaskGetTickCount();
                    oled_show_countdown(selected_laps, 3);
                }
                goto skip_output;
            }

            /* ---- 3秒倒计时 ---- */
            case MOTOR_STATE_COUNTDOWN:
            {
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;

                uint32_t elapsed = xTaskGetTickCount() - state_ticks;
                if (elapsed >= pdMS_TO_TICKS(3000)) {
                    /* 倒计时结束，开始循迹 */
                    OLED_Clear();
                    oled_show_running(selected_laps, lost_count);
                    state = MOTOR_STATE_TRACKING;
                } else {
                    /* 每秒刷新一次倒计时数字 */
                    uint8_t sec_left = 3 - (uint8_t)(elapsed / pdMS_TO_TICKS(1000));
                    oled_show_countdown(selected_laps, sec_left);
                }
                goto skip_output;
            }

            /* ---- 正常循迹 ---- */
            case MOTOR_STATE_TRACKING:
            {
                if (!line_found) {
                    lost_count++;
                    oled_show_running(selected_laps, lost_count);
                    if (lost_count >= lost_limit) {
                        state = MOTOR_STATE_STOPPED;
                    } else {
                        state = MOTOR_STATE_LOST_STRAIGHT;
                        state_ticks = xTaskGetTickCount();
                    }
                    break;
                }

                float steering_output = Steering_PID_Compute(
                    &gSteeringPID, 0.0f, track_result.track_error, dt);

                float target_l = g_base_speed + steering_output;
                float target_r = g_base_speed - steering_output;

                float pid_l = Speed_PID_Compute(&gMotorLeftPID,  target_l, g_encoder_left.speed_mps,  dt);
                float pid_r = Speed_PID_Compute(&gMotorRightPID, target_r, g_encoder_right.speed_mps, dt);

                duty_l = ramp_duty(out_l, pid_l);
                duty_r = ramp_duty(out_r, pid_r);
                break;
            }

            /* ---- 丢线后直行 0.3 秒 ---- */
            case MOTOR_STATE_LOST_STRAIGHT:
            {
                uint32_t elapsed = xTaskGetTickCount() - state_ticks;
                if (elapsed >= pdMS_TO_TICKS(100)) {
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

            /* ---- 原地左转（右轮转，左轮停）直到找线 ---- */
            case MOTOR_STATE_LOST_TURN:
            {
                if (line_found) {
                    state = MOTOR_STATE_TRACKING;
                    break;
                }
                out_l = 0.0f;
                Motor_SetPWM(0, 0.0f);

                float pid_r = Speed_PID_Compute(&gMotorRightPID, g_base_speed, g_encoder_right.speed_mps, dt);
                out_r = ramp_duty(out_r, pid_r);
                Motor_SetPWM(1, out_r);
                goto skip_output;
            }

            /* ---- 完成指定圈数，停止 ---- */
            case MOTOR_STATE_STOPPED:
            default:
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;
                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t *)"Done!           ", 16);
                OLED_ShowString(0, 2, (uint8_t *)"Press B26 again ", 16);
                /* 等待 B26 再次确认重置 */
                state = MOTOR_STATE_IDLE;
                oled_show_select(selected_laps);
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

    xTaskCreate(motorTask, "motorTask", 0x300, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
