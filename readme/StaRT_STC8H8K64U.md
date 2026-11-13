# StaRT on STC8H8K64U-45I-LQFP64

该移植使用 SDCC 的 MCS-51 大内存模型与 `--stack-auto`，复用 StaRT 的优先级调度、
同优先级时间片、线程休眠、软件定时器和计数信号量。示例配置使用 8 个优先级
和 100 Hz tick，关闭互斥量、消息队列、微秒延时和调试日志。

## 构建和烧录

依赖 CMake 3.20+、Ninja 和 SDCC（含 `sdas8051` 及 MCS-51 库）。
Ubuntu 24.04 / WSL 可安装以下软件包。

```sh
sudo apt-get install cmake ninja-build sdcc sdcc-libraries
cmake --preset stc8h8k64u
cmake --build --preset stc8h8k64u
```

使用 STC-ISP 打开 `build/stc8h8k64u/StaRT-stc8h8k64u.hex` 并选择实际型号。
同目录的 `.map` 和 `.mem` 可用于检查链接布局。默认时钟为 **45 MHz**，请在
STC-ISP 选择相同的 IRC 频率。若使用其他频率，重新配置并构建，例如
`cmake --preset stc8h8k64u -DSTART_STC8H_CLOCK_HZ=24000000`。

通过 UART1 的 P3.1 (TX) 观察串口输出，串口设为 115200、8N1，并共地。
预期先打印 StaRT 启动横幅，随后约每秒输出一组 `BA`。
生产者输出 `B` 后释放信号量，消费者获得信号量后输出 `A`。
信号量支持立即尝试、无限等待和按 tick 限时等待，超时沿用 StaRT 的 `S_ERR`。
Timer 1 采用 STC 的 16 位自动重载模式产生 tick，Timer 2 驱动 UART1 波特率。
Timer 0 保持停止，软件写 `TF0` 请求在较低优先级的中断里切换线程。
芯片寄存器和定时器模式见 [STC8H 系列技术手册](https://www.stcmicro.com/datasheet/STC8H-221114.pdf)。

## 线程栈和中断

用 `static __xdata s_uint8_t stack[START_STC8H_STACK_IMAGE_SIZE]` 为每个线程
预留栈镜像缓冲区。切换时，移植层保存寄存器，把有效硬件栈复制到旧线程的
XDATA 缓冲区，再将新线程的有效栈复制回片内 RAM。缓冲区前两个字节分别记录
容量和有效字节数，切换时发现超出容量会停在 `s_mcs51_stack_fault`。
硬件栈溢出如果发生在进入切换中断之前，切换检查可能来不及捕获。
示例为每个线程预留 256 字节 XDATA，链接器为活动硬件栈预留 192 字节。

线程代码和调用内核的模块须使用相同的 SDCC 内存模型和 `--stack-auto`。
此移植使用寄存器组 0 和 DPTR0，应用需保持默认 DPTR 模式。Timer 0 应低于
调用内核的中断优先级。当前 BSP 只有 tick 中断调用内核，应用中断应通过
标志通知线程处理，阻塞 API 仅在线程中调用。预算硬件栈时须计入嵌套中断。

## 模拟器验证

```sh
sudo apt-get install sdcc-ucsim python3
cmake --preset stc8h8k64u -DSTART_STC8H_SIM_TESTS=ON
cmake --build --preset stc8h8k64u
ctest --test-dir build/stc8h8k64u --output-on-failure
```

测试检查板级固件的复位、Timer 0 与 Timer 1 中断向量，并在 uCsim 的 8052
模型中运行相同的内核与汇编切换代码。测试覆盖首次派发、优先级抢占、时间片、
主动让出、休眠唤醒、线程返回、关中断嵌套、短栈保护，以及信号量释放唤醒、
超时和删除时取消等待。模拟器使用 8052 的
Timer 1 重载方式，STC 的自动重载、UART1 输出和切换延迟需要实板验证。

固件的 Flash 和 XDATA 占用记录在链接器的 `.mem` 报告中。
uCsim 会报告测试的硬件栈峰值，应用的栈占用取决于调用深度和中断负载。
