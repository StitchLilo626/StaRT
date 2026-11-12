#!/usr/bin/env python3
"""Execute the real GNU port/startup on QEMU's Cortex-M3 MPS2 core model."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="arm-none-eabi-gcc")
    parser.add_argument("--qemu", default="qemu-system-arm")
    parser.add_argument("--optimization", choices=("O0", "Os"), default="Os")
    parser.add_argument("--output", type=Path, default=ROOT / "build/port-smoke")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    elf = output / "port-smoke.elf"
    poison = output / "ram-poison.bin"
    poison.write_bytes(b"\xa5" * (20 * 1024))
    sources = ["tests/port/smoke.c", "tests/port/registers.S", "libcpu/CM3/cpuport.c",
               "libcpu/CM3/arm_gcc.S", "bsp/stm32/stm32f103c8/GNU/startup_stm32f103c8.S"]
    command = [args.compiler, "-mcpu=cortex-m3", "-mthumb", "-mfloat-abi=soft",
               "-std=c99", "-" + args.optimization, "-g", "-Wall", "-Wextra", "-Werror",
               "-fno-common", "-ffunction-sections", "-fdata-sections", "-nostartfiles",
               "--specs=nano.specs", "--specs=nosys.specs", "-Wl,--gc-sections,-z,noexecstack",
               "-T" + str(ROOT / "tests/port/mps2.ld"), "-I" + str(ROOT / "include"),
               "-I" + str(ROOT / "bsp/stm32/stm32f103c8/Core/Inc"),
               *[str(ROOT / source) for source in sources], "-o", str(elf)]
    try:
        subprocess.run(command, check=True)
        result = subprocess.run([args.qemu, "-M", "mps2-an385", "-cpu", "cortex-m3",
                                 "-nographic", "-monitor", "none", "-serial", "none",
                                 "-device", "loader,file={},addr=0x20000000,force-raw=on".format(poison),
                                 "-semihosting-config", "enable=on,target=native",
                                 "-kernel", str(elf)], stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=10, check=False)
        print(result.stdout, end="")
        if result.returncode or "PORT_SMOKE_PASS" not in result.stdout:
            raise RuntimeError("Cortex-M3 port smoke test failed")
        print("PASS: startup, constructors, first dispatch, PRIMASK nesting, "
              "pending-switch replacement, r4-r11 restore, PSP and spurious PendSV")
        print("QEMU MPS2 core test only; the STM32 HAL/peripherals are not emulated.")
        return 0
    except (OSError, subprocess.SubprocessError, RuntimeError) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
