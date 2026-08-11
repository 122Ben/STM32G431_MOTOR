# STM32G431 无刷电机六步换相控制工程

基于 STM32G431（Keil MDK-uVision5，HAL 库）的无刷直流电机（BLDC）**六步换相**控制工程，霍尔传感器驱动，三路运放电流采样做过流保护，母线电压欠压/过压保护，按键启停调速，USART2 上位机调试协议，转速 PID 闭环调节。

## 1. 硬件引脚映射

| 功能 | 引脚 | 说明 |
|---|---|---|
| TIM1_CH1 / CH2 / CH3 | PA8 / PA9 / PA10 | 三相桥上管 PWM，AF6 |
| TIM1_CH1N | PC13 | 下管互补输出，**AF4**（与 CH2N 不同！） |
| TIM1_CH2N | PA12 | 下管互补输出，AF6 |
| TIM1_CH3N | PB15 | 下管互补输出，**AF4** |
| HALL1 / HALL2 / HALL3 | PB8 / PB7 / PB6 | 霍尔输入，内部上拉，双边沿中断 |
| VBUS_ADC | PA0 | ADC1_IN1，母线电压分压采样 |
| BUTTON | PC10 | 按键，内部上拉，下降沿中断，假定低有效 |
| TXD2 / RXD2 (USART2) | PB3 / PB4 | AF7，115200-8N1 |
| Curr_fd1 OPAMP1 +/- | PA1 / PA3 | U 相电流，OPAMP1_OUT 内部接 ADC1_IN3 |
| Curr_fd2 OPAMP2 +/- | PA7 / PA5 | V 相电流，OPAMP2_OUT 内部接 ADC2_IN3 |
| Curr_fd3 OPAMP3 +/- | PB0 / PB2 | W 相电流，OPAMP3_OUT 内部接 ADC1_IN12 |

以上 AF 编号、OPAMP 内部到 ADC 通道的映射均已对照 ST 官方引脚数据库交叉核实（TIM1_CH1N/CH3N 与 CH2N 的 AF 编号不同，是常见的踩坑点）。

工程目标芯片型号为 **STM32G431RBTx（LQFP64，128KB Flash / 32KB RAM）**：64 脚封装才能完整引出 PC10、PC13 等引脚。如果实际芯片是其他型号/封装（如 CBT6/CBU6），在 Keil 里重新选择 Device 即可，不需要改代码。

时钟完全基于内部 HSI16（未使用外部晶振，引脚表里没有 OSC_IN/OSC_OUT），通过 PLL 得到 170MHz 主频（HSI16/4×85/2）。

## 2. 目录结构

```
MDK-ARM/BLDC_SixStep.uvprojx   Keil5 工程文件
Core/Inc, Core/Src             应用代码 + CubeMX 风格外设初始化
Core/Startup                   启动文件 (原厂 startup_stm32g431xx.s)
Drivers/CMSIS                  原厂 CMSIS Core + STM32G4 Device 头文件/system 文件
Drivers/STM32G4xx_HAL_Driver   原厂 HAL 库（仅保留本工程用到的模块）
```

`Drivers/` 和 `Core/Startup` 下的文件均为 ST 官方 `STM32CubeG4` / `stm32g4xx_hal_driver` / `cmsis_device_g4` 仓库的原始源码（未做任何修改），不是手写模拟版本。

## 3. 用 Keil5 打开与编译

1. 确认已安装 **Keil::STM32G4xx_DFP** Device Family Pack（Pack Installer 里搜索 STM32G4 安装，或者升级到最新版本也可以）。工程里写的 Pack 版本号如果和你本机安装的不完全一致，Keil 会提示，用 "Manage Run-Time Environment" 或者直接在 Device 栏重新选一次 `STM32G431RBTx` 即可，不影响源码。
2. 双击 `MDK-ARM/BLDC_SixStep.uvprojx` 用 Keil5 打开。
3. Build (F7)。工程用 AC6 (ARM Compiler 6) 编译器。

> 本仓库是在没有 Keil/ARMCC 工具链的 Linux 沙箱环境里生成的，**没有条件实际跑一次 Keil 编译**做最终验证。已经逐个对照拉取到的原厂 HAL/CMSIS 头文件核实了用到的结构体字段名、宏名、寄存器位名，但仍建议第一次打开工程后完整 Build 一遍，如果报错欢迎反馈。

## 4. 上电前必读 / 调试步骤

这几件事离不开在实物上验证，代码里给的是保守的占位值，**不要直接一次性上高压/高限流试机**：

1. **换相方向**：`Core/Inc/motor_config.h` 里的 `MOTOR_HALL_TABLE_SELECT` 决定换相表方向。手工焊的板子，霍尔和绕组的相位关系事先无法确定。第一次上电建议先限制母线电压/限流（比如用可调电源限流到 0.5~1A），用 `D100`（10% 占空比开环）命令让电机轻转：
   - 如果电机平顺转动 -> 方向表选对了；
   - 如果电机卡顿、异响、电流大但不转 -> 把 `MOTOR_HALL_TABLE_SELECT` 改成 `MOTOR_HALL_TABLE_REVERSE` 重新编译烧录。
2. **过流阈值**：`CURRENT_ADC_ZERO`（零电流时的 ADC 读数）和 `CURRENT_ADC_TRIP_DELTA`（触发保护的偏移量）需要用示波器/万用表在实物上标定，电流采样运放增益由板子上外部电阻决定，代码里没有假设具体增益。
3. **母线电压分压比**：`VBUS_DIVIDER_RATIO` 默认按 10k:1k（11倍）占位，需要按实际分压电阻改。
4. **极对数**：`MOTOR_POLE_PAIRS` 只影响转速换算和 PID，不影响换相本身，务必改成实际电机的极对数，否则转速读数和 PID 调节效果都不对。
5. PWM 频率 20kHz，中心对齐，死区约 600ns（`TIM1_DEADTIME_DTG`），如果 MOSFET/驱动 IC 需要更长死区，按 `motor_config.h` 里的公式重新计算。

## 5. UART2 命令协议（115200-8N1，行结尾 `\n`）

| 命令 | 说明 |
|---|---|
| `S` | 启动电机（闭环转速 PID，目标转速由 `R` 设置） |
| `X` | 停止电机 |
| `C` | 清除故障锁存（过流/欠压/过压触发后需要先清除才能重新启动） |
| `R<rpm>` | 设置闭环目标转速，例：`R3000` |
| `D<permille>` | 开环占空比测试模式，0~1000（0.1%步进），例：`D200` = 20% 占空比，用于换相方向标定 |
| `?` | 立即请求一次遥测 |

遥测（每 `TELEMETRY_PERIOD_MS`=200ms 自动发送一次，格式）：

```
T VBUS=12.400V IU=2048 IV=2050 IW=2049 RPM=3012 TARGET=3000 STATE=RUNNING FAULT=NONE
```

按键 PC10：短按启停，长按（≥800ms）循环切换预设转速档（1000/2000/3000/4000/5000/0 RPM）。

## 6. 已知限制

- 六步换相，非 FOC，效率/平顺性不如矢量控制，但实现和调试都简单得多。
- 电流采样是软件定时顺序扫描（~1kHz），不是 FOC 那种与 PWM 中心同步的三相瞬时采样，只用于过流保护/监控，不做瞬时力矩重构。
- 过流保护是软件轮询触发（TIM1 MOE 位关断），响应时间取决于采样周期（约 1ms 量级），如果需要更快的硬件级保护，需要在电路上接入比较器到 TIM1 的 BKIN 并在 `tim.c` 里把 `BreakState` 改为使能。
