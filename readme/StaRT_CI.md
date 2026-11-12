# Cortex-M3 C 源码编译检查

本页描述 C 源码配置矩阵。新增的完整固件链接、产物检查和 QEMU 核心测试见
[STM32F103C8 GNU 固件构建](StaRT_GNU_FIRMWARE.md)。

`.github/workflows/repository.yml` 在原有仓库检查之外增加
`Cortex-M3 C compile (O0/Os)` 两个任务，每次 push 和 pull request 执行。
无需开发板、Keil 许可证、CMake、模拟器或目标 C 运行库。检查由 Python 3.9+
标准库驱动 GNU Arm 编译器，真实生成 ARM 目标文件；任何编译告警或错误都会
使任务失败。**通过只表示所选配置下的 C 源码可编译，不表示固件构建成功。**

## 工程评估

基线为 `8c50d904fc1538c80a9f99f8022e31f27a1b7e69`。现有 Keil 工程：

- `bsp/stm32/stm32f103c8/MDK-ARM/StaRT-stm32f103c8.uvprojx`，目标 `StaRT`。
- ARM Compiler 5.06 update 7 (build 960)，`uAC6=0`，C99，`Optim=4`、
  `wLevel=2`，设备包 `Keil.STM32F1xx_DFP.2.4.1`。
- 预定义 `USE_HAL_DRIVER,STM32F103xB`；包含 Core/Inc、HAL/Legacy、
  CMSIS 设备头文件、CMSIS/Include 和仓库 include。内核 C 文件自身只依赖
  `start.h`、`sdef.h`、`StaRT_Config.h` 及编译器提供的标准类型/可变参数头文件，
  因此本检查无需 HAL 宏和厂商头文件。
- `libcpu/CM3/cpuport.c` 已有 `__GNUC__` 分支，使用 `__builtin_ffs`。
  检查同时编译 `START_USING_CPU_FFS=0/1` 两条条件路径；在 GCC 下两者均
  使用 builtin，不能据此验证 ARMCC 的内嵌汇编实现。
- `libcpu/CM3/arm.s` 使用 ARMASM 的 `IMPORT/EXPORT/AREA/PROC/ENDP` 等指令。
  板级 `startup_stm32f103xb.s` 同样使用 ARMASM 语法，并跳转到 ARM 运行库
  `__main`。它们不能直接由 GNU assembler 编译。该基线尚无 GNU 链接脚本、
  GNU 启动及运行库配置；后续独立 GNU 构建见上方链接。

本次仅为公开 API 补齐既有实现的函数原型和标准头文件依赖，并将两个文件
内部初始化函数设为 `static`；不改变调度算法，不修改 Keil 工程、ARMASM
文件、板级配置或 `Drivers/` 中的第三方代码。

## 本地执行

Ubuntu 24.04（与 CI 相同的发行版）：

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends -y gcc-arm-none-eabi
python3 scripts/check_kernel_compile.py
```

Windows 安装 GNU Arm Embedded Toolchain 后同样可以执行：

```powershell
python scripts/check_kernel_compile.py
```

只验证某组配置、使用指定编译器或查看完整命令：

```sh
python3 scripts/check_kernel_compile.py --profile bsp --optimization Os --verbose
python3 scripts/check_kernel_compile.py --compiler /opt/toolchain/bin/arm-none-eabi-gcc
```

`--profile` 和 `--optimization` 均可重复。不指定时运行全部配置和两个优化级别。
`--compiler` 是单个可执行文件路径；不接受含参数的 shell 命令。
默认输出位于已忽略的 `build/cortex-m3/<profile>/<optimization>/`，包含目标文件、
依赖文件、生成的配置头文件和头文件探针。`report.json` 记录编译器版本、
配置覆盖、逐文件完整命令和返回码；发生编译错误时也生成报告并返回非零状态。
`--output` 可指定其他输出目录。不要把生成物提交到 Git。

CI 使用固定发行版 `ubuntu-24.04` 的 apt 工具链，输出实际编译器版本。
包版本随发行版维护更新，并非锁定的二进制工具链；复现问题时应保存版本和报告。

## 验证范围

每个配置/优化组合编译：

1. `src/` 下全部 C 源码，当前 7 个文件。
2. `libcpu/CM3/` 下全部 C 源码，当前为 `cpuport.c`。
3. `include/` 下每个公开头文件单独作为首个 include，并重复 include，
   防止依赖包含顺序、遗漏标准头文件或缺失包含保护。
4. 一个仅供编译的 API 探针：检查整数/指针宽度、线程 PSP 成员偏移，
   调用线程、定时器、IPC、延时和 idle hook API，并展开消息池及日志宏。
   探针本身不会执行，也不参与 Keil 工程。

新加入上述源码目录的 C 文件会自动纳入检查。当前每组是 8 个实现文件、
2 个头文件探针和 1 个 API 探针，共 11 次编译；12 个配置乘以两个优化
级别，共 **264 次编译**。

使用 `-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c99 -ffreestanding`，
以及 `-fno-common -ffunction-sections -fdata-sections`。告警选项包括
`-Wall -Wextra -Werror -Wstrict-prototypes -Wmissing-prototypes`
`-Wold-style-definition -Wundef -Wshadow -Wformat=2`。这些选项检查声明、
原型、未定义条件宏、常见类型及告警问题；它们不等于覆盖 GCC 所有诊断，
自定义 `s_printf` 也没有标准 printf 的完整格式检查属性。

## 配置矩阵

`bsp` 使用仓库真实的 `Core/Inc/StaRT_Config.h`。其他配置生成包装头文件，
先包含该真实文件，再逐项 `#undef/#define`。不能仅用命令行 `-D` 覆盖，
因为板级头文件中的无条件定义会将它重新定义。原始文件始终保持原样。
下面未列出的开关继承 BSP 默认值；实际覆盖项以脚本的 `PROFILES` 为准。

| 配置 | 覆盖内容 |
| --- | --- |
| bsp | 原始板级配置 |
| minimal | IPC 及三个子功能、CPU FFS、DEBUG、UDELAY、idle hooks 全关；8 优先级、100 Hz、32 字节输出缓冲 |
| ipc-empty | IPC 开启，三个子功能全关 |
| semaphore-only | 仅信号量；idle hooks 关闭 |
| mutex-only | 仅互斥锁；CPU FFS 关闭 |
| messagequeue-only | 仅消息队列；16 优先级、UDELAY 关闭 |
| no-semaphore | IPC 开启，仅互斥锁和消息队列；DEBUG 关闭 |
| no-mutex | IPC 开启，仅信号量和消息队列 |
| no-messagequeue | IPC 开启，仅信号量和互斥锁 |
| ipc-all | IPC 及三个子功能、CPU FFS、DEBUG、UDELAY 全开；4 个 idle hooks |
| ipc-parent-off | IPC 关闭但三个子开关保持开启，检查父开关屏蔽关系 |
| limits | 1 个优先级、4 层 timer 链表、1 个 idle hook、100 Hz、128 字节 idle 栈 |

矩阵覆盖 IPC 子功能的全部 8 种组合，以及父开关关闭、常见裁剪和部分数值
边界。`limits` 仅验证类型/数组声明可编译，不证明多层 timer 算法正确，也不
证明 128 字节栈足以实际运行。没有穷举所有宏和所有数值。

## 尚未验证

- ARMCC/ARMASM 分支、IRQ 保存恢复、PendSV 上下文切换及启动向量。
- BSP 应用 C 文件和 HAL/CMSIS 厂商源码的整体构建。
- 最终链接、未解析符号、跨文件 ABI 一致性、运行库依赖和 Flash/RAM 布局。
  单个文件生成 `.o` 可以保留未解析外部符号；本检查没有用桩函数隐藏这些缺口。
- Keil 的实际重新构建、上板、栈水位、抢占/并发/定时器/IPC 行为与性能。

完整 GNU 固件构建后续需要单独提供 GNU 汇编移植、启动文件、链接脚本及
运行库配置，并验证切换与中断行为。当前 Actions 任务名称和日志均明确说明
是 C 编译检查，不能把绿色结果记录为“STM32 固件构建通过”。

## 2026-10-15 本地验证记录

- Windows GNU Arm Embedded Toolchain 10.3-2021.10，GCC 10.3.1：
  12 配置 × 2 优化 × 11 文件 = 264 次 C 编译，零失败、零告警。
- WSL Ubuntu 24.04 的 `gcc-arm-none-eabi` 包 `15:13.2.rel1-2`，GCC 13.2.1：
  同样 264 次 C 编译，零失败、零告警。包及 binutils 解压在本地构建目录运行，
  未在 GitHub 托管 runner 中实际执行本次工作流。
- `readelf` 确认样本输出为 ELF32 ARM EABI5 可重定位目标文件，M-profile、
  Thumb-2，确实进行了交叉编译。
- 四项反向验证均返回预期的非零状态：编译器路径不存在、删除 `sdef.h` 的
  `stddef.h` 依赖、制造 `s_putc` 声明/定义冲突，以及新增仅在
  `messagequeue-only` 配置报错的 C 文件。最后一项同时验证自动发现新源码。
  临时错误全部恢复。
- GNU assembler 试编译现有两个 ARMASM 文件均失败，符合上述兼容性评估。
  失败用于确认验证边界，不计入 C 编译矩阵的通过结果。
- 原有 6 项 Python 单元测试、仓库检查、`git diff --check` 通过；
  actionlint 1.7.11 验证工作流通过。
- 本机没有可调用的 Keil/ARMCC，未重新构建 Keil 固件；未上板。

参考：[GCC ARM 目标选项](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html)、
[GCC 告警选项](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)。
