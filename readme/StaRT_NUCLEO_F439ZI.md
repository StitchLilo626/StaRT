# StaRT on NUCLEO-F439ZI

## 构建和烧录

安装 CMake 3.20+、Ninja、Python 3.9+ 和带 newlib-nano 的 GNU Arm 工具链。
在仓库根目录执行以下命令。

```sh
cmake --preset nucleo-f439zi
cmake --build --preset nucleo-f439zi
```

输出位于 `build/nucleo-f439zi/`，包含 `StaRT-nucleo-f439zi.elf`、`.hex`、
`.bin`、`.map` 以及 `firmware-report.json`。

1. 将电脑连接到板载 ST-LINK 的 USB 接口（CN1），保持板载调试跳帽为默认状态。
2. 在 STM32CubeProgrammer 中选择 ST-LINK，通过 SWD 连接并下载
   `StaRT-nucleo-f439zi.hex`。使用 BIN 时，下载地址填 `0x08000000`。
3. 打开 ST-LINK 的虚拟串口，设置为 115200、8N1、无流控，然后复位开发板。

板载虚拟串口使用 USART3（PD8 TX、PD9 RX），出厂跳线配置下只需要一根 USB
数据线。具体板级连线见 [ST UM1974](https://www.st.com/resource/en/user_manual/dm00244518-stm32-nucleo-144-boards-mb1137-stmicroelectronics.pdf)。

## 上电预期

- 串口打印 StaRT 横幅与 `NUCLEO-F439ZI, HCLK 168 MHz`，随后每约 500 ms
  输出一行 `heartbeat`。
- 生产者每 500 ms 翻转绿灯 LD1（PB0）并释放计数信号量。消费者取到信号量后
  点亮蓝灯 LD2（PB7）约 100 ms。

## 移植配置

- 内部 16 MHz HSI 经 PLL 产生 168 MHz HCLK，APB1 为 42 MHz，APB2 为
  84 MHz。PLL 就绪超时时以 16 MHz HSI 运行，串口会打印实际配置的时钟。
- SysTick 提供 1 kHz 节拍，PendSV 使用最低优先级。固件使用
  `libcpu/arm/CM4F` 和 FPv4-SP hard-float。线程首次启动使用基本异常帧。
  浮点线程使用扩展异常帧，由硬件保存 S0–S15 和 FPSCR，由 PendSV 保存
  S16–S31，并随线程保存 EXC_RETURN。浮点上下文比基本帧多 136 字节。
- 2 MiB Flash 从 `0x08000000` 开始，192 KiB 连续 SRAM 从 `0x20000000`
  开始，预留 2 KiB MSP 栈。64 KiB CCM 暂未分配。寄存器和内存布局见
  [ST RM0090](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)。

## 验证

构建自动核对 ELF、HEX 和 BIN，包括向量表、强定义的 SysTick/PendSV 处理
函数与内存边界。安装 `qemu-system-arm` 后可运行 Cortex-M4 核心测试。

```sh
python3 scripts/check_cpu_port.py --cpu cortex-m4 --output build/port-m4
```

QEMU 使用 MPS2-AN386 的 Cortex-M4 模型，验证启动、基本帧与扩展帧之间的
切换，以及 S0–S31 和 FPSCR 恢复。板上的 PLL、
LED、USART3 和 ST-LINK 下载需实板验证。
