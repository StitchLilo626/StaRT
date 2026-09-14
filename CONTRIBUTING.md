# Contributing to StaRT

StaRT 借鉴 Linux 内核的补丁管理方式：一个提交解决一个问题，正文说明
原因、影响和验证结果，每个提交都应能独立审查、回退和用于 `git bisect`。
这里采用 `type(scope): summary` 标题，这是本项目对 Linux 子系统前缀的
调整，不是声称 Linux 内核本身采用 Conventional Commits。

## 开发流程

从最新的 `master` 创建短期分支，例如：

```sh
git fetch origin
git switch -c fix/timer-timeout origin/master
```

合并前检查分支内的每个提交。历史提交保持原样，不为统一标题而改写已发布
历史。只整理尚未共享的本地提交；有明确原因时可以使用合并提交。

先修正确性，再做性能优化；功能、修复、格式整理和厂商库升级分别提交。
移动文件时保持内容不变，再另行修改，方便追踪历史。

## 提交信息

标题不超过 72 个字符，不加句号，使用简短的英文祈使句：

```text
fix(timer): reject invalid timeout arguments

Explain the failing condition and its user-visible impact here.
Explain why this change resolves it and mention relevant trade-offs.

Validation: describe the actual checks, board and compiler used.
```

这是格式示例，不代表此问题已经修复或验证。

支持的类型：`fix`、`feat`、`docs`、`perf`、`refactor`、`test`、`build`、
`ci`、`chore`、`style`、`revert`。文档统一使用 `docs`，不混用 `doc`。
scope 可省略；推荐 `sched`、`thread`、`timer`、`ipc`、`cm3`、`bsp`、`git`。
标题和正文用空行分隔，正文写明动机，通常每行约 72 字符。

修复已定位的历史回归时，加入真实的原始提交和标题：

```text
Fixes: <至少12位提交ID> ("原始提交标题")
```

不要猜测 `Fixes:`，不要虚构 `Tested-by:`、`Reviewed-by:` 或验证结果。
Linux 使用 `Signed-off-by:` 表示对 DCO 的认证；本项目暂不强制 DCO。
只有贡献者理解并能作出该认证时才用 `git commit -s`，不代替他人签署。

可选的本地模板设置（仅作用于此仓库）：

```sh
git config --local commit.template .gitmessage
```

## 提交哪些文件

- 保留源码、启动汇编、`.ioc`、`.uvprojx`、`.uvoptx` 和 `.scvd` 等工程输入。
- 构建目录、`.lst`、`.map`、目标文件、固件镜像和个人调试状态不进入 Git。
- `Drivers/` 中的预编译 CMSIS 库是随仓库提供的依赖，不作为构建垃圾删除。
- 第三方代码升级单独提交，注明来源与版本，保留其许可证和原始格式。
- `document/` 中已有的版本化 PDF 保留为历史文档；日常文档修改优先更新
  `readme/` 的 Markdown。不要在每次构建时提交导出的 PDF。
- `.env` 和 `.codegraph/` 是本地文件。忽略规则不等于秘密扫描；提交前
  仍应检查暂存区的实际内容。

通用忽略规则放根目录，板级输出目录放 BSP 自己的 `.gitignore`。
新增源码或工程输入后，用 `git check-ignore --no-index <path>` 确认未被
误忽略。不要直接使用 `git add .` 代替检查，优先 `git add -p` 或明确路径。

## 格式与许可证

自有代码沿用当前四空格缩进和现有括号风格，不为模拟 Linux 的八列 Tab
而批量重排内核。`.editorconfig` 和 `.gitattributes` 固定文本约定。
格式修改单独提交；不要对整个厂商目录执行格式化或换行归一化。

StaRT 自有代码按 [MIT](LICENSE) 发布。`Drivers/` 内的第三方文件继续遵守
各自许可证；根目录许可证不覆盖它们。

## 验证要求

提交前检查暂存区，提交后检查分支范围（Python 3.9+，仅使用标准库）：

```sh
git diff --cached --check
git diff --cached
python scripts/check_repository.py --base origin/master
```

检查脚本验证新增非合并提交的标题与正文、每个提交的空白错误，以及索引中
是否混入被忽略的文件。旧历史不追溯套用新格式。GitHub Actions 在 PR 中
检查目标分支到 PR HEAD 的提交范围；普通推送只运行基础检查。

当前示例工程：
`bsp/stm32/stm32f103c8/MDK-ARM/StaRT-stm32f103c8.uvprojx`，目标 `StaRT`。
工程记录 ARM Compiler 5.06 update 7 (build 960) 和
`Keil.STM32F1xx_DFP.2.4.1`。在装有所需工具的 Windows 环境中可以执行：

```text
UV4.exe -b bsp/stm32/stm32f103c8/MDK-ARM/StaRT-stm32f103c8.uvprojx -t StaRT
```

记录实际编译器、优化级别、配置和构建结果。仓库检查通过不代表固件编译
通过，编译通过也不代表时序和并发正确。没有工具链或开发板时，明确写出
未验证项，不能写成“全部测试通过”。

内核修改的验证应覆盖相关场景：

- 调度：优先级抢占、同优先级轮转、空闲线程、PendSV 连续请求。
- 临界区：嵌套中断、恢复原有中断屏蔽状态、最坏中断关闭时间。
- 定时器：tick 回绕、零延时、边界值、取消与到期竞争。
- IPC：释放与超时竞争、删除时唤醒、互斥锁优先级继承与恢复。
- 线程：重复启动、自删除、重启、栈对齐和栈水位。

性能提交提供同一板卡、频率、编译器和优化等级下的前后数据：切换周期数、
最坏中断延迟、Flash/RAM 用量、测试负载及测量方法。不要只报平均值。

## 参考

- [Linux: Submitting patches](https://www.kernel.org/doc/html/latest/process/submitting-patches.html)
- [Linux: .gitignore](https://github.com/torvalds/linux/blob/master/.gitignore)
- [Developer Certificate of Origin](https://developercertificate.org/)
