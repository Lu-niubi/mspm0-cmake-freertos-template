#ifndef TRACKING_H
#define TRACKING_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

// 传感器数据结构
typedef struct {
    uint8_t L1, L2, L3, L4;
    uint8_t R1, R2, R3, R4;
} TrackSensorData;

// 循迹状态
typedef enum {
    TRACK_LEFT,
    TRACK_RIGHT,
    TRACK_CENTER,
    TRACK_LOST
} TrackState;

// 循迹结果
typedef struct {
    float line_position;
    float track_error;
    TrackState state;
    uint8_t sensor_pattern;
} TrackResult;

// 函数声明
void Track_ReadSensors(TrackSensorData *data);
TrackResult CalculateTrackResult(TrackSensorData *data);

#endif
