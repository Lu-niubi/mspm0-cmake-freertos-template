#include "Tracking.h"
#include <math.h>
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include "ti_msp_dl_config.h"

static float my_fabsf(float x) { return (x < 0.0f) ? -x : x; }

// 传感器位置权重 - 物理布局从左到右: L1(最外) L2 L3 L4(最内) | R4(最内) R3 R2 R1(最外)
// pattern bit顺序: bit7=L4, bit6=L3, bit5=L2, bit4=L1, bit3=R1, bit2=R2, bit1=R3, bit0=R4
// L4/R4在中间→权重小(±1)，L1/R1在外侧→权重大(±4)
const float SENSOR_POSITIONS[8] = {
    -1.0f, -2.0f, -3.0f, -4.0f,  // L4(bit7), L3(bit6), L2(bit5), L1(bit4)
    +4.0f, +3.0f, +2.0f, +1.0f   // R1(bit3), R2(bit2), R3(bit1), R4(bit0)
};

void Track_ReadSensors(TrackSensorData *data)
{
    if (!data) return;

    // 使用配置的头文件宏定义读取引脚
    data->L1 = (DL_GPIO_readPins(Tracking_L1_PORT, Tracking_L1_PIN) != 0) ? 1 : 0;
    data->L2 = (DL_GPIO_readPins(Tracking_L2_PORT, Tracking_L2_PIN) != 0) ? 1 : 0;
    data->L3 = (DL_GPIO_readPins(Tracking_L3_PORT, Tracking_L3_PIN) != 0) ? 1 : 0;
    data->L4 = (DL_GPIO_readPins(Tracking_L4_PORT, Tracking_L4_PIN) != 0) ? 1 : 0;

    // data->R1 = (DL_GPIO_readPins(Tracking_R1_PORT, Tracking_R1_PIN) != 0) ? 1 : 0;
    data->R1 = 0; // R1传感器损坏，暂时固定为0
    data->R2 = (DL_GPIO_readPins(Tracking_R2_PORT, Tracking_R2_PIN) != 0) ? 1 : 0;
    data->R3 = (DL_GPIO_readPins(Tracking_R3_PORT, Tracking_R3_PIN) != 0) ? 1 : 0;
    data->R4 = (DL_GPIO_readPins(Tracking_R4_PORT, Tracking_R4_PIN) != 0) ? 1 : 0;
}

TrackResult CalculateTrackResult(TrackSensorData *data) {
    TrackResult result = {0};
    if (!data) return result;

    // 生成8位传感器掩码（匹配物理布局顺序）
    uint8_t pattern =
        (data->L4 << 7) |  // L4 (bit7)
        (data->L3 << 6) |  // L3 (bit6)
        (data->L2 << 5) |  // L2 (bit5)
        (data->L1 << 4) |  // L1 (bit4)
        (data->R1 << 3) |  // R1 (bit3)
        (data->R2 << 2) |  // R2 (bit2)
        (data->R3 << 1) |  // R3 (bit1)
        data->R4;          // R4 (bit0)

    // 中心模式检测（L4和R4同时检测，它们是最靠近中间的传感器）
    const uint8_t CENTER_PATTERN = 0x81; // 二进制:10000001 = L4(bit7)+R4(bit0)

    if (pattern == CENTER_PATTERN) {
        result.state = TRACK_CENTER;
        result.line_position = 0.0f;
    } else {
        float weighted_sum = 0.0f;
        uint8_t active_count = 0;

        // 遍历所有传感器
        for (int i = 0; i < 8; i++) {
            // 检查当前传感器是否激活
            if (pattern & (0x80 >> i)) {
                weighted_sum += SENSOR_POSITIONS[i];
                active_count++;
            }
        }

        if (active_count > 0) {
            // 计算中心线偏移位置（负值表示偏左，正值表示偏右）
            result.line_position = weighted_sum / active_count;

            // 确定跟踪状态
            if (my_fabsf(result.line_position) < 0.1f) {
                result.state = TRACK_CENTER;
            } else {
                result.state = (result.line_position < 0) ? TRACK_LEFT : TRACK_RIGHT;
            }
        } else {
            // 没有检测到任何传感器
            result.state = TRACK_LOST;
            result.line_position = 0.0f;
        }
    }

    // 返回计算结果
    result.track_error = result.line_position;
    result.sensor_pattern = pattern;
    return result;
}
