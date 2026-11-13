#!/usr/bin/env python3
"""Execute the GNU port and STM32 startup on QEMU's Cortex-M MPS2 models."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PORTS = {
    "cortex-m3": ("mps2-an385", "bsp/stm32/stm32f103c8/Core/Inc",
                  "bsp/stm32/stm32f103c8/GNU/startup_stm32f103c8.S"),
    "cortex-m4": ("mps2-an386", "bsp/stm32/nucleo-f439zi",
                  "bsp/stm32/nucleo-f439zi/startup_stm32f439zi.S"),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="arm-none-eabi-gcc")
    parser.add_argument("--qemu", default="qemu-system-arm")
    parser.add_argument("--cpu", choices=PORTS, default="cortex-m3")
    parser.add_argument("--optimization", choices=("O0", "Os"), default="Os")
    parser.add_argument("--output", type=Path, default=ROOT / "build/port-smoke")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    elf = output / "port-smoke.elf"
    poison = output / "ram-poison.bin"
    poison.write_bytes(b"\xa5" * (20 * 1024))
    machine, config, startup = PORTS[args.cpu]
    cm4f = args.cpu == "cortex-m4"
    port = "libcpu/arm/CM4F" if cm4f else "libcpu/arm/CM3"
    float_flags = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"] if cm4f else ["-mfloat-abi=soft"]
    sources = ["tests/port/smoke.c", "tests/port/registers.S", port + "/cpuport.c",
               port + "/arm_gcc.S", startup]
    command = [args.compiler, "-mcpu=" + args.cpu, "-mthumb", *float_flags,
               "-std=c99", "-" + args.optimization, "-g", "-Wall", "-Wextra", "-Werror",
               "-fno-common", "-ffunction-sections", "-fdata-sections", "-nostartfiles",
               "--specs=nano.specs", "--specs=nosys.specs", "-Wl,--gc-sections,-z,noexecstack",
               "-T" + str(ROOT / "tests/port/mps2.ld"), "-I" + str(ROOT / "include"),
               "-I" + str(ROOT / config),
               "-I" + str(ROOT / port),
               *[str(ROOT / source) for source in sources], "-o", str(elf)]
    try:
        subprocess.run(command, check=True)
        result = subprocess.run([args.qemu, "-M", machine, "-cpu", args.cpu,
                                 "-nographic", "-monitor", "none", "-serial", "none",
                                 "-device", "loader,file={},addr=0x20000000,force-raw=on".format(poison),
                                 "-semihosting-config", "enable=on,target=native",
                                 "-kernel", str(elf)], stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=10, check=False)
        print(result.stdout, end="")
        if result.returncode or "PORT_SMOKE_PASS" not in result.stdout:
            raise RuntimeError(f"{args.cpu} port smoke test failed")
        print("PASS: startup, constructors, first dispatch, PRIMASK nesting, "
              "pending-switch replacement, r4-r11 restore, PSP and spurious PendSV")
        if cm4f:
            print("PASS: basic/extended frames, s0-s31 and FPSCR across floating-point threads")
        print("QEMU MPS2 core test only; the STM32 HAL/peripherals are not emulated.")
        return 0
    except (OSError, subprocess.SubprocessError, RuntimeError) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
