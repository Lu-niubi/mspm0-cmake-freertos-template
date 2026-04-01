#include "motor_task.h"
#include "motor_driver.h"
#include "encoder.h"
#include "tracking_task.h"
#include "button.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

QueueHandle_t xMotorSpeedQueue = NULL;
Speed_PID_Controller gMotorLeftPID;
Speed_PID_Controller gMotorRightPID;

float g_base_speed = 1.2f;

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
                Speed_PID_Init(&gMotorLeftPID,  600.0f, 150.0f, 0.0f, 800.0f, 0.8f);
                Speed_PID_Init(&gMotorRightPID, 600.0f, 150.0f, 0.0f, 800.0f, 0.8f);

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
                if (elapsed >= pdMS_TO_TICKS(100)) {
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
                if (line_found) {
                    state = MOTOR_STATE_TRACKING;
                    break;
                }
                out_r = 0.0f;
                Motor_SetPWM(1, 0.0f);

                out_l = Speed_PID_Compute(&gMotorRightPID, g_base_speed, g_encoder_left.speed_mps, dt);
                Motor_SetPWM(0, out_l);
                goto skip_output;
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

    Speed_PID_Init(&gMotorLeftPID,  600.0f, 150.0f, 0.0f, 800.0f, 0.8f);
    Speed_PID_Init(&gMotorRightPID, 600.0f, 150.0f, 0.0f, 800.0f, 0.8f);

    xMotorSpeedQueue = xQueueCreate(1, sizeof(MotorSpeed_t));

    xTaskCreate(motorTask, "motorTask", 0x300, NULL,
                configMAX_PRIORITIES - 2, NULL);
}
