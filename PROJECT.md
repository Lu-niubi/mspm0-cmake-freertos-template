# MSPM0 FreeRTOS CMake Template

基于 TI MSPM0G3507 微控制器的 FreeRTOS 项目模板，使用 CMake 构建系统和 ARM GCC 工具链。

## 项目结构

```
mspm0-cmake-freertos-template/
├── CMakeLists.txt                  # 主 CMake 构建配置
├── mspm0g350x_base.cmake           # MCU 工具链与编译器配置
├── Sources/                        # 应用源代码
│   ├── main.c                      # 程序入口，FreeRTOS 任务创建
│   ├── sysmem.c                    # 堆管理 (_sbrk 实现)
│   └── uart_printf.c              # UART 调试输出工具
├── Includes/                       # 应用头文件
│   ├── FreeRTOSConfig.h            # FreeRTOS 内核配置 (详细)
│   └── uart_printf.h              # UART printf 声明
├── FreeRTOS/                       # FreeRTOS 内核 v11.1.0
│   ├── *.c                         # 内核核心文件 (tasks, queue, list 等)
│   ├── portable/
│   │   ├── GCC/ARM_CM0/            # Cortex-M0+ 移植层
│   │   └── MemMang/heap_4.c       # 动态内存管理 (带合并的堆分配)
│   └── include/                    # FreeRTOS 头文件
├── SysConfig/                      # TI SysConfig 生成的设备配置
│   ├── MSPM0_FreeRTOS_Template.syscfg  # SysConfig 工程文件
│   ├── ti_msp_dl_config.h/c       # 外设初始化代码
│   ├── device_linker.lds          # GCC 链接脚本
│   └── device.opt                 # 设备编译宏定义
├── .vscode/                        # VS Code 开发环境配置
│   ├── settings.json
│   ├── tasks.json                 # 构建/烧录任务
│   └── launch.json                # 调试配置
└── build/                          # CMake 构建输出目录
```

## 硬件目标

| 项目 | 说明 |
|------|------|
| 目标芯片 | **MSPM0G3507** (LP_MSPM0G3507 LaunchPad) |
| 内核架构 | ARM Cortex-M0+ (ARMv6-M) |
| Flash | 128 KB (`0x00000000 - 0x00020000`) |
| SRAM | 32 KB (`0x20200000 - 0x20208000`) |
| 系统时钟 | 80 MHz |

## 工具链

- **编译器**: ARM GCC (`arm-none-eabi-gcc/g++`)
- **构建系统**: CMake ≥ 3.30 + Unix Makefiles
- **烧录工具**: OpenOCD (支持 XDS110 / CMSIS-DAP 调试器)
- **MSPM0 SDK**: v2.05.01.00 (默认路径: `D:/Keil/mspm0_sdk_2_05_01_00/`)
- **SysConfig**: v1.24.2

## FreeRTOS 配置

| 配置项 | 值 |
|--------|-----|
| 版本 | v11.1.0 |
| Tick 频率 | 1000 Hz (1ms) |
| 最大优先级 | 8 |
| 最小栈大小 | 128 words (512 bytes) |
| 堆大小 | 8 KB (heap_4) |
| 抢占式调度 | 启用 |
| 时间片 | 禁用 |
| 静态分配 | 启用 |
| 动态分配 | 启用 |
| 软件定时器 | 启用 |

## 构建方式

### 配置

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -G "Unix Makefiles" -B build
```

### 编译

```bash
cmake --build build --target all
```

构建产物: `build/MSPM0_FreeRTOS_Template.elf`

### 编译优化级别

| 模式 | 标志 | 说明 |
|------|------|------|
| Debug | `-Og -g` | 最小优化 + 调试信息 |
| Release | `-Ofast` | 最大速度优化 |
| RelWithDebInfo | `-Ofast -g` | 速度优化 + 调试信息 |
| MinSizeRel | `-Os` | 最小体积优化 |

## 烧录

### XDS110 调试器

```bash
openocd -s <scripts_path> -c "adapter speed 4000" \
        -f interface/xds110.cfg -f target/ti_mspm0.cfg \
        -c "program build/MSPM0_FreeRTOS_Template.elf verify reset exit"
```

### CMSIS-DAP 调试器

```bash
openocd -s <scripts_path> -c "adapter speed 4000" \
        -f interface/cmsis-dap.cfg -f target/ti_mspm0.cfg \
        -c "program build/MSPM0_FreeRTOS_Template.elf verify reset exit"
```

## 应用程序概览

`main.c` 中创建了两个示例任务：

| 任务 | 优先级 | 栈大小 | 功能 |
|------|--------|--------|------|
| `printLogTask` | 7 | 0x80 words | 每秒打印内存使用情况 |
| `blinkTask` | 7 | 0x80 words | 每 100ms 翻转 LED |

```c
int main() {
    SYSCFG_DL_init();  // 初始化 MSPM0 外设

    xTaskCreate(printLogTask, "printLogTask", 0x80, NULL, 7, &handle);
    xTaskCreate(blinkTask, "blinkTask", 0x80, NULL, 7, &handle);

    vTaskStartScheduler();  // 启动调度器 (不应返回)
}
```

## 外部依赖

| 依赖 | 来源 | 说明 |
|------|------|------|
| MSPM0 DriverLib | MSPM0 SDK | TI 外设驱动库 |
| CMSIS Core | MSPM0 SDK (third_party) | ARM CMSIS 头文件 |
| FreeRTOS v11.1.0 | 项目内置 | 实时操作系统内核 |
| Newlib Nano | ARM GCC 自带 | 精简 C 标准库 |

## SysConfig 模块

当前通过 SysConfig 配置了以下外设模块：

- **Board** - 开发板配置
- **GPIO** - 通用 IO
- **I2C** - I2C 总线
- **MATHACL** - 数学加速器
- **SYSCTL** - 系统控制

如需修改外设配置，编辑 `SysConfig/MSPM0_FreeRTOS_Template.syscfg` 并重新生成。

## 注意事项

- SDK 路径在 `mspm0g350x_base.cmake` 第 40 行配置，如安装位置不同请修改
- 链接脚本中最小堆/栈均为 256 bytes，FreeRTOS 任务栈由内核独立管理
- `heap_4.c` 提供带合并功能的动态内存分配，堆大小 8KB
- 编译使用 `--specs=nano.specs` 和 `-nostartfiles`，启动代码来自 SDK
