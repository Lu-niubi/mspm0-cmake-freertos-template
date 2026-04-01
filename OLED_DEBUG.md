# OLED 移植 Debug 记录

## 环境
- 芯片：MSPM0G3507，80MHz
- 编译：GCC + CMake（-O2）
- 驱动：软件模拟 I2C（bit-bang），SSD1306，128×64

## 引脚配置
| 信号 | GPIO | PINCM |
|------|------|-------|
| SCL  | GPIOA.31 | PINCM6 |
| SDA  | GPIOA.28 | PINCM3 |

SysConfig 中已配置为推挽输出，初始低电平。

## 遇到的三个 Bug

### Bug 1：`oled_show_select()` 末尾多余的 `OLED_Clear()`
**现象**：选圈界面一片黑，内容写完立刻被清除。
**原因**：函数末尾多了一行 `OLED_Clear()`，把刚写的内容全部擦掉。
**修复**：删除末尾多余的 `OLED_Clear()`。

```c
// 错误版本
OLED_ShowString(0, 4, (uint8_t *)"A26+ A25- B26OK", 16);
OLED_Clear();  // ← 多余，立刻擦屏
}

// 正确版本
OLED_ShowString(0, 4, (uint8_t *)"A26+ A25- B26OK", 16);
}
```

### Bug 2：`OLED_Init` 使用 `vTaskDelay`
**现象**：OLED 初始化可能在 FreeRTOS 任务切换期间被打断。
**原因**：`vTaskDelay(pdMS_TO_TICKS(200))` 会让出 CPU，上电稳定延迟期间可能切换到其他任务，但更大的问题是 bit-bang I2C 时序依赖 CPU 连续执行。
**修复**：改为裸机忙等：

```c
// 错误版本
vTaskDelay(pdMS_TO_TICKS(200));

// 正确版本（80MHz × 0.2s = 16,000,000 cycles）
delay_cycles(16000000UL);
```

### Bug 3（根本原因）：I2C 时序太快，SSD1306 采样失败
**现象**：OLED 完全不亮，无任何响应。
**原因**：GCC -O2 优化下，GPIO 翻转只需数 ns，SCL 高/低电平宽度远低于 SSD1306 要求的 **≥400ns**，I2C 通信完全无效。参考例程在 Keil + 未优化环境下靠 Flash 读取延迟自然满足时序，GCC 优化环境下会暴露此问题。
**修复**：在所有 SCL 翻转处加入 `delay_cycles(80)`（约 1μs，I2C 约 500kHz）：

```c
#define I2C_DELAY()  delay_cycles(80)  // 80MHz / 80 = 1μs

static void Send_Byte(uint8_t dat)
{
    for (uint8_t i = 0; i < 8; i++) {
        OLED_SCL_Clr();
        I2C_DELAY();
        if (dat & 0x80) OLED_SDA_Set();
        else            OLED_SDA_Clr();
        I2C_DELAY();
        OLED_SCL_Set();
        I2C_DELAY();
        OLED_SCL_Clr();
        dat <<= 1;
    }
}
```

I2C_Start / I2C_Stop / I2C_WaitAck 同样在 SCL 翻转前后加入 `I2C_DELAY()`。

## 结论
> **核心教训**：将裸机 bit-bang 驱动移植到高主频 + 编译器优化环境时，必须手动插入时序延迟，不能依赖执行时间自然满足 I2C 规范。
