# STM32F103C8 完整 GNU 固件构建

此构建复用现有板级应用、HAL/CMSIS 和全部内核 C 源码，使用新增的 GNU
上下文切换汇编、启动文件和链接脚本，完成真实固件链接并导出 ELF、HEX、BIN
及 map 文件。无需开发板或 Keil 许可证。它与原有的
[C 源码配置矩阵检查](StaRT_CI.md) 分别运行。

## 构建和产物

Ubuntu 24.04 安装依赖：

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends -y cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi
```

从仓库根目录构建（CMake 3.20+、Python 3.9+）：

```sh
cmake -S . -B build/stm32-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/stm32-debug --parallel 2

cmake -S . -B build/stm32-size -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build/stm32-size --parallel 2
```

Windows 安装 GNU Arm Embedded Toolchain、CMake、Ninja 和 Python 后使用同样
命令；PowerShell 中将两个 `-D...` 参数分别用引号包住。不指定构建类型时
默认为 `MinSizeRel`。工具链可执行文件需要位于 PATH。

每个构建目录包含 `StaRT-stm32f103c8.elf/.hex/.bin/.map`、
`firmware-report.json` 和 `compile_commands.json`。构建自动检查产物，发现
错误即失败。也可独立运行：

```sh
python3 scripts/check_firmware.py build/stm32-debug/StaRT-stm32f103c8.elf
python3 tests/firmware/check_regressions.py build/stm32-debug/StaRT-stm32f103c8.elf
```

所有输出均放在已忽略的 `build/` 中。源码和构建输入进入 Git，固件镜像不进入 Git。

## GNU 移植与内存布局

- `libcpu/CM3/arm_gcc.S` 与 Keil 的 `arm.s` 独立。保存/恢复 r4–r11 和 PSP，
  使用 PRIMASK 临界区及最低优先级 PendSV。首次派发不返回；连续待处理请求
  保留最初的 outgoing PSP，只更新最后的目标。发布切换请求时保存并恢复原
  PRIMASK；没有软件切换请求的 PendSV 保留原 EXC_RETURN。
- `GNU/startup_stm32f103c8.S` 提供 59 项中断向量：16 项内核向量和 43 项外设
  向量。Reset 复制 `.data`、清零 `.bss`、设置 VTOR，再调用 `SystemInit`、
  newlib 构造器和现有 `main`。PendSV/SysTick 没有 weak 默认实现，遗漏真实
  处理函数会链接失败。其他未实现中断进入 `Default_Handler`。
- `GNU/stm32f103c8.ld` 使用器件规定的 **64 KiB Flash、20 KiB RAM**，
  Flash 起始 `0x08000000`，RAM 起始 `0x20000000`，初始 MSP `0x20005000`。
  栈顶预留 1024 字节，链接器拒绝静态数据覆盖 MSP 栈。这里没有 bootloader
  偏移，也没有为部分芯片可能存在的额外 Flash 扩大容量。
- 使用 Cortex-M3 Thumb、soft-float、C99、newlib-nano 和 nosys，不启用
  semihosting。`s_printf` 继续通过原应用的 UART `s_putc` 输出。没有提供
  动态 heap 或操作系统 syscall 实现；nosys 不能当作可用的文件 I/O 或 malloc
  支持。此静态示例不需要这些功能。
- 保留 Keil 工程、ARMASM 启动和原移植代码；`Drivers/` 和 ST 生成区不变。
  唯一现有 BSP 源码改动位于自有 `USER CODE`：将 `DWT_Delay_us` 参数改成
  `s_uint32_t`，与内核 hook 原型一致。GNU newlib 的 `uint32_t` 与内核别名
  可能对应不同的 C 基本类型，不能用“宽度相同”代替函数指针类型兼容。

## 三层验证

1. 保留原有 12 配置 × O0/Os 的 C 编译检查，验证条件编译组合。
2. 完整固件以 Debug 和 MinSizeRel 构建，编译 24 个 C 单元和 2 个汇编单元，
   包含真实板级应用和与 Keil 工程相同的 12 个 HAL 单元。C 告警、汇编告警和
   链接告警作为错误；未使用函数可由 `--gc-sections` 移除。产物检查确认 ARM
   ELF32/EABI、Thumb 入口、向量、强符号、无未解析符号、Flash/RAM 及启动边界，
   并逐字节核对 BIN/HEX 与 ELF 的 Flash 装载内容。11 个反向检查涵盖坏镜像、
   Flash 溢出、RAM/栈重叠和缺失关键中断处理函数。
3. `scripts/check_cpu_port.py` 用 QEMU `mps2-an385` 的 Cortex-M3 核心执行真实
   GNU 启动、`cpuport.c` 和 `arm_gcc.S`，使用独立测试应用和 MPS2 链接布局。
   先把 20 KiB RAM 填为非零值，验证 `.data/.bss`、构造器顺序、首次派发、
   PRIMASK 嵌套、连续请求合并、r4–r11 恢复、PSP 及无切换请求的 PendSV。
   该测试在 O0/Os 下均执行，超时或非零退出都会失败。

运行核心测试需要额外安装 `qemu-system-arm`：

```sh
python3 scripts/check_cpu_port.py --optimization O0
python3 scripts/check_cpu_port.py --optimization Os
```

现有 GitHub Actions 增加两个固件任务和一个 QEMU 核心任务，全部通过才算本次
新增检查通过。固件任务上传镜像、map 和报告，保留 14 天；原仓库检查及内核
C 配置矩阵继续运行。

## 2026-10-15 本地实际结果

| 工具链 | 构建类型 | Flash 镜像字节 | 静态 RAM 字节 | MSP 预留字节 |
| --- | --- | ---: | ---: | ---: |
| Windows GCC 10.3.1 | Debug | 11784 | 2504 | 1024 |
| Windows GCC 10.3.1 | MinSizeRel | 7388 | 2512 | 1024 |
| Ubuntu GCC 13.2.1 | Debug | 11948 | 2504 | 1024 |
| Ubuntu GCC 13.2.1 | MinSizeRel | 7544 | 2512 | 1024 |

四种构建均完成链接和产物检查，无编译/链接告警。两版工具链执行了镜像/链接
反向验证；QEMU 8.2.2 使用 GCC 13.2.1，在 O0/Os 下通过核心测试。另临时删除
`.bss` 清零或寄存器保存指令，测试均能报错；临时错误已恢复。原有六项 Python
测试、仓库检查和 actionlint 通过。云端运行结果以 PR 的 Actions 记录为准。

## 验证边界

本次可以称为“完整 STM32F103C8 GNU 示例固件构建通过”。QEMU 测试运行的是
核心测试镜像，**不是 STM32 固件的外设仿真**。尚未验证 STM32 的实际 8 MHz
HSE/72 MHz PLL、UART、GPIO、DWT、时序、中断最坏延迟或完整 RTOS 并发行为；
也未测量运行栈水位。1024 字节 MSP 和现有线程栈只是配置，不是安全容量证明。
本机没有 Keil/ARMCC，未重新执行 Keil 构建；其工程输入和厂商代码保持原样。

参考：[STM32F103x8/xB 数据手册](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)、
[QEMU MPS2 文档](https://www.qemu.org/docs/master/system/arm/mps2.html)。
