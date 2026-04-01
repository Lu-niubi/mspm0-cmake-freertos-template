#include "motor_task.h"
#include "motor_driver.h"
#include "encoder.h"
#include "tracking_task.h"
#include "button.h"
#include "task.h"
#include "queue.h"
#include <math.h>
#include <stdio.h>

QueueHandle_t xMotorSpeedQueue = NULL;
Speed_PID_Controller gMotorLeftPID;
Speed_PID_Controller gMotorRightPID;

float g_base_speed = 1.2f;
static float my_fabsf(float x) { return (x < 0.0f) ? -x : x; }

/* ---- 辅助：向 oledTask 发送显示消息 ---- */
static void oled_post_select(uint8_t laps)
{
    OledMsg_t msg = { .type = OLED_MSG_SELECT, .laps = laps };
    xQueueOverwrite(xOledQueue, &msg);
}

static void oled_post_countdown(uint8_t laps, uint8_t sec)
{
    OledMsg_t msg = { .type = OLED_MSG_COUNTDOWN, .laps = laps, .sec_left = sec };
    xQueueOverwrite(xOledQueue, &msg);
}

static void oled_post_running(uint8_t laps, uint8_t lost_count)
{
    OledMsg_t msg = { .type = OLED_MSG_RUNNING, .laps = laps, .lost_count = lost_count };
    xQueueOverwrite(xOledQueue, &msg);
}

static void oled_post_done(uint8_t laps)
{
    OledMsg_t msg = { .type = OLED_MSG_DONE, .laps = laps };
    xQueueOverwrite(xOledQueue, &msg);
}

typedef enum {
    MOTOR_STATE_IDLE,
    MOTOR_STATE_COUNTDOWN,
    MOTOR_STATE_TRACKING,
    MOTOR_STATE_LOST_STRAIGHT,
    MOTOR_STATE_LOST_TURN,
    MOTOR_STATE_LOST_CENTERING,  /* 找到线后对中，直到误差足够小再恢复循迹 */
    MOTOR_STATE_STOPPED,
} MotorState_t;

void motorTask(void *pvParameters)
{
    (void)pvParameters;

    TrackResult track_result;
    MotorState_t state     = MOTOR_STATE_IDLE;
    uint8_t lost_count     = 0;
    uint8_t lost_limit     = 4;
    uint8_t selected_laps  = 1;
    uint32_t state_ticks   = 0;
    TickType_t last_lost_ticks = 0;          /* 上次确认丢线的时刻 */
    const uint32_t lost_debounce_ms = 3000;  /* 两次丢线最小间隔 */
    const float dt = 0.02f;

    float out_l = 0.0f;
    float out_r = 0.0f;

    oled_post_select(selected_laps);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

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
            case MOTOR_STATE_IDLE:
            {
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;
                Speed_PID_Init(&gMotorLeftPID,  450.0f, 150.0f, 0.0f, 800.0f, 0.8f);
                Speed_PID_Init(&gMotorRightPID, 450.0f, 150.0f, 0.0f, 800.0f, 0.8f);

                bool need_refresh = false;
                if (btn.a26_edge)
                {
                    selected_laps++;
                    if (selected_laps > 9) selected_laps = 1;
                    need_refresh = true;
                }
                if (btn.a25_edge)
                {
                    if (selected_laps > 1) selected_laps--;
                    need_refresh = true;
                }
                if (need_refresh)
                {
                    oled_post_select(selected_laps);
                }

                if (btn.b26_edge)
                {
                    lost_limit  = selected_laps * 4;
                    lost_count  = 0;
                    state       = MOTOR_STATE_COUNTDOWN;
                    state_ticks = xTaskGetTickCount();
                    oled_post_countdown(selected_laps, 3);
                }
                goto skip_output;
            }

            case MOTOR_STATE_COUNTDOWN:
            {
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;

                uint32_t elapsed = xTaskGetTickCount() - state_ticks;
                if (elapsed >= pdMS_TO_TICKS(3000)) {
                    oled_post_running(selected_laps, lost_count);
                    state = MOTOR_STATE_TRACKING;
                } else {
                    uint8_t sec_left = 3 - (uint8_t)(elapsed / pdMS_TO_TICKS(1000));
                    oled_post_countdown(selected_laps, sec_left);
                }
                goto skip_output;
            }

            case MOTOR_STATE_TRACKING:
            {
                if (!line_found) {
                    TickType_t now = xTaskGetTickCount();
                    uint32_t since_last = (uint32_t)(now - last_lost_ticks);
                    if (since_last < pdMS_TO_TICKS(lost_debounce_ms)) {
                        /* 距上次丢线不足3秒，忽略，继续循迹 */
                        duty_l = Speed_PID_Compute(&gMotorLeftPID,  g_base_speed, g_encoder_left.speed_mps,  dt);
                        duty_r = Speed_PID_Compute(&gMotorRightPID, g_base_speed, g_encoder_right.speed_mps, dt);
                        break;
                    }
                    last_lost_ticks = now;
                    lost_count++;
                    oled_post_running(selected_laps, lost_count);
                    if (lost_count >= lost_limit) {
                        state = MOTOR_STATE_STOPPED;
                        goto skip_output;
                    }
                    state = MOTOR_STATE_LOST_STRAIGHT;
                    state_ticks = xTaskGetTickCount();
                    duty_l = out_l;
                    duty_r = out_r;
                    break;
                }

                float steering_output = Steering_PID_Compute(
                    &gSteeringPID, 0.0f, track_result.track_error, dt);

                float target_l = g_base_speed + steering_output;
                float target_r = g_base_speed - steering_output;

                duty_l = Speed_PID_Compute(&gMotorLeftPID,  target_l, g_encoder_left.speed_mps,  dt);
                duty_r = Speed_PID_Compute(&gMotorRightPID, target_r, g_encoder_right.speed_mps, dt);
                break;
            }

            case MOTOR_STATE_LOST_STRAIGHT:
            {
                uint32_t elapsed = xTaskGetTickCount() - state_ticks;
                if (elapsed >= pdMS_TO_TICKS(150)) {
                    state = MOTOR_STATE_LOST_TURN;
                    state_ticks = xTaskGetTickCount();
                    break;
                }
                duty_l = Speed_PID_Compute(&gMotorLeftPID,  g_base_speed, g_encoder_left.speed_mps,  dt);
                duty_r = Speed_PID_Compute(&gMotorRightPID, g_base_speed, g_encoder_right.speed_mps, dt);
                break;
            }

            case MOTOR_STATE_LOST_TURN:
            {
                /* 找到线且误差足够小（接近中心）才切回正常循迹 */
                if (line_found && my_fabsf(track_result.track_error) < 2.5f) {
                    state = MOTOR_STATE_TRACKING;
                    break;
                }
                /* 无论是否找到线，一直保持原地转弯直到对中 */
                /* 转弯速度用 base_speed 的一半，避免冲过头 */
                const float turn_speed = g_base_speed * 0.5f;
                out_r = 0.0f;
                Motor_SetPWM(1, 0.0f);

                out_l = Speed_PID_Compute(&gMotorRightPID, turn_speed, g_encoder_left.speed_mps, dt);
                Motor_SetPWM(0, out_l);
                goto skip_output;
            }

            case MOTOR_STATE_LOST_CENTERING:
            {
                /* 此状态已合并到 MOTOR_STATE_LOST_TURN，不再使用 */
                state = MOTOR_STATE_LOST_TURN;
                break;
            }

            case MOTOR_STATE_STOPPED:
            default:
                Motor_Stop();
                out_l = 0.0f;
                out_r = 0.0f;
                oled_post_done(selected_laps);
                state = MOTOR_STATE_IDLE;
                oled_post_select(selected_laps);
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

    Speed_PID_Init(&gMotorLeftPID,  450.0f, 150.0f, 0.0f, 800.0f, 0.8f);
    Speed_PID_Init(&gMotorRightPID, 450.0f, 150.0f, 0.0f, 800.0f, 0.8f);

    xMotorSpeedQueue = xQueueCreate(1, sizeof(MotorSpeed_t));

    xTaskCreate(motorTask, "motorTask", 0x300, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
