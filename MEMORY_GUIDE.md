# MSPM0G3507 FreeRTOS 内存调整说明

## 硬件资源上限

| 资源 | 大小 |
|------|------|
| SRAM | 32 KB |
| Flash | 128 KB |
| FreeRTOS Heap 实现 | heap_4.c（支持碎片合并） |

---

## 本次修改内容与原因

### 1. `Includes/FreeRTOSConfig.h` — 增大 FreeRTOS Heap

```c
// 修改前
#define configTOTAL_HEAP_SIZE  (1024 * 8)   // 8 KB

// 修改后
#define configTOTAL_HEAP_SIZE  (1024 * 20)  // 20 KB
```

**为什么：**
原来只分配了 8 KB 给 FreeRTOS heap，但 SRAM 一共有 32 KB，.data/.bss 和 C 运行时栈只占约 3-4 KB，剩余空间完全浪费。
每创建一个任务会从 heap 消耗：任务栈（自定义大小）+ TCB 结构体（约 300 字节）。
原来 8 KB 最多容纳约 6-8 个小任务，增加到 20 KB 后可以容纳更多任务，还有余量给队列、信号量等内核对象。

> 安全原则：configTOTAL_HEAP_SIZE ≤ 24 KB（给 .bss/.data/C栈保留至少 8 KB）

---

### 2. `Includes/FreeRTOSConfig.h` — 开启栈溢出检测

```c
// 修改前
#define configCHECK_FOR_STACK_OVERFLOW  0

// 修改后
#define configCHECK_FOR_STACK_OVERFLOW  2
```

**为什么：**
栈溢出是嵌入式系统最常见的隐性 bug——任务栈分配太小时，局部变量会覆盖相邻内存，导致程序随机崩溃，极难调试。
设为 `2` 时，FreeRTOS 在每次任务切换时检查栈末尾的"填充图案"是否被破坏，一旦检测到立即调用 `vApplicationStackOverflowHook`，可以直接定位到是哪个任务溢出。

配套在 `main.c` 中实现了回调：
```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    uart_printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    while(1);  // 停在这里方便调试器定位
}
```

---

### 3. `Includes/FreeRTOSConfig.h` — 开启 Trace 功能 + 栈水位函数

```c
// 修改前
#define configUSE_TRACE_FACILITY                0
#define INCLUDE_uxTaskGetStackHighWaterMark     0

// 修改后
#define configUSE_TRACE_FACILITY                1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
```

**为什么：**
`uxTaskGetStackHighWaterMark()` 需要两个宏同时开启才能编译：
- `configUSE_TRACE_FACILITY = 1`：让 FreeRTOS 在 TCB 结构体中记录追踪信息
- `INCLUDE_uxTaskGetStackHighWaterMark = 1`：将该函数编译进固件（FreeRTOS 的可选 API 默认不编译）

该函数返回任务从创建到现在**栈剩余的最小值**（单位：words），是确定任务栈是否够用的唯一可靠手段。
返回值越大越安全，建议保持 > 20 words 的余量。

---

### 4. `Sources/main.c` — printLogTask 栈从 512B 增大到 1 KB

```c
// 修改前
xTaskCreate(printLogTask, "printLogTask", 0x80, ...);  // 128 words = 512 bytes

// 修改后
xTaskCreate(printLogTask, "printLogTask", 0x100, ...); // 256 words = 1024 bytes
```

**为什么：**
`printLogTask` 调用 `uart_printf`，底层的格式化函数（`printf` 系列）会在栈上分配较大的临时缓冲区。
128 words（512 字节）对于带 printf 的任务非常危险，极容易溢出。
256 words（1 KB）是使用 uart/串口打印任务的推荐最小值。

blinkTask 只做 GPIO 翻转，128 words 完全够用，保持不变。

---

### 5. `Sources/main.c` — 扩展运行时内存监控

```c
void printLogTask()
{
    while (true)
    {
        uart_printf("Free heap: %d bytes\r\n", xPortGetFreeHeapSize());
        uart_printf("Min ever heap: %d bytes\r\n", xPortGetMinimumEverFreeHeapSize());
        uart_printf("printLogTask stack watermark: %d words\r\n",
                    uxTaskGetStackHighWaterMark(printLogTask_handle));
        uart_printf("blinkTask stack watermark: %d words\r\n",
                    uxTaskGetStackHighWaterMark(blinkTask_handle));
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
```

**为什么：**
- `xPortGetFreeHeapSize()`：当前 heap 剩余量，实时监控内存压力
- `xPortGetMinimumEverFreeHeapSize()`：历史最低值，反映峰值内存占用，比当前值更重要——如果这个值很低，说明曾经接近耗尽
- `uxTaskGetStackHighWaterMark()`：任务栈剩余最小水位，判断栈是否需要调大

---

## 任务栈大小选取指南

| 任务类型 | 建议栈大小 | 说明 |
|---------|-----------|------|
| 简单逻辑（GPIO、延时） | 128 words (512B) | 无函数嵌套，无格式化 |
| 串口打印（uart_printf） | 256 words (1KB) | printf 格式化需要临时缓冲 |
| 使用 sprintf/snprintf | 512 words (2KB) | 格式化输出到字符串 |
| 软件浮点运算 | 额外 +128 words | M0+ 无 FPU，浮点在栈上模拟 |
| 复杂业务逻辑 | 从 512 words 起步 | 根据水位监控结果调整 |

## 内存预算（当前配置）

```
32 KB SRAM
├── .data / .bss / C栈    ≈ 3-4 KB
└── FreeRTOS Heap         20 KB
    ├── printLogTask 栈    1 KB  + TCB ~300B
    ├── blinkTask 栈       512B  + TCB ~300B
    ├── Idle Task 栈       512B
    ├── Timer Task 栈      512B
    └── 剩余可用           ≈ 16 KB（可再创建约 10 个简单任务）
```

## 调整 heap 大小的安全公式

```
所需 heap = (每个任务栈 bytes) + (任务数 × 300B TCB)
           + 队列/信号量等内核对象
           + 20% 安全余量

configTOTAL_HEAP_SIZE ≤ 32 KB - .bss/.data估算(≈4KB) - C栈(256B)
                       ≤ 约 27 KB 上限（保守取 24 KB）
```
