#!/usr/bin/env python3
"""Verify image validation and linker rejection using a real built firmware."""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from check_firmware import Elf, validate_elf, validate_hex  # noqa: E402


def rejected(name, operation):
    try:
        operation()
    except (ValueError, KeyError, struct.error):
        print("PASS: reject " + name)
        return
    raise RuntimeError("Accepted invalid image: " + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("--compiler", default="arm-none-eabi-gcc")
    args = parser.parse_args()
    data = args.elf.read_bytes()
    elf = Elf(data)
    report, image = validate_elf(data)
    validate_hex(args.elf.with_suffix(".hex").read_text(), image, int(report["entry"], 16))
    directory = args.elf.resolve().parent / "negative-checks"
    directory.mkdir(parents=True, exist_ok=True)

    rejected("truncated ELF", lambda: validate_elf(data[:30]))
    mutations = [
        ("initial stack outside RAM", elf.sections[".isr_vector"][4], 0x20010000),
        ("wrong reset entry", 24, elf.entry ^ 1),
        ("weak default PendSV vector", elf.sections[".isr_vector"][4] + 14 * 4,
         elf.symbol("Default_Handler")),
        ("reserved vector in use", elf.sections[".isr_vector"][4] + 7 * 4,
         elf.symbol("Reset_Handler")),
    ]
    # First program header is the Flash PT_LOAD segment in this firmware.
    phoff = struct.unpack_from("<I", data, 28)[0]
    if struct.unpack_from("<I", data, phoff)[0] != 1:
        raise RuntimeError("Expected a Flash load segment for mutation fixture")
    mutations.append(("Flash load overflow", phoff + 12, 0x08010000))
    for name, offset, value in mutations:
        corrupted = bytearray(data)
        struct.pack_into("<I", corrupted, offset, value)
        rejected(name, lambda: validate_elf(bytes(corrupted)))
    hex_text = args.elf.with_suffix(".hex").read_text()
    bad_checksum = hex_text[:1] + ("1" if hex_text[1] != "1" else "2") + hex_text[2:]
    rejected("HEX checksum/length", lambda: validate_hex(bad_checksum, image, elf.entry))

    # Exercise the public command, including its nonzero exit on a stale BIN.
    test_elf = directory / "bad-bin.elf"
    for suffix in (".elf", ".bin", ".hex", ".map"):
        shutil.copyfile(args.elf.with_suffix(suffix), test_elf.with_suffix(suffix))
    binary = bytearray(test_elf.with_suffix(".bin").read_bytes())
    binary[-1] ^= 1
    test_elf.with_suffix(".bin").write_bytes(binary)
    result = subprocess.run([sys.executable, str(ROOT / "scripts/check_firmware.py"),
                             str(test_elf)], capture_output=True, text=True)
    if result.returncode != 1 or "BIN does not match ELF" not in result.stderr:
        raise RuntimeError("Mismatched BIN was not rejected")
    print("PASS: reject BIN mismatch with nonzero exit")

    linker = ROOT / "bsp/stm32/stm32f103c8/GNU/stm32f103c8.ld"
    fixture = (
        ".syntax unified\n.cpu cortex-m3\n.thumb\n"
        ".section .isr_vector,\"a\"\n.word _estack,Reset_Handler\n.space 57*4\n"
        ".section .text,\"ax\"\n.global Reset_Handler\n.thumb_func\n"
        "Reset_Handler: b Reset_Handler\n"
    )
    for name, extra, diagnostic in (
        ("flash-overflow", ".space 65536\n", "overflowed"),
        ("ram-stack-overlap", ".section .bss\n.space 20000\n", "overlaps reserved MSP stack"),
    ):
        source = directory / (name + ".S")
        source.write_text(fixture + extra, encoding="utf-8")
        result = subprocess.run([args.compiler, "-mcpu=cortex-m3", "-mthumb", "-nostdlib",
                                 "-Wl,-z,noexecstack", "-T" + str(linker), str(source),
                                 "-o", str(directory / (name + ".elf"))],
                                capture_output=True, text=True)
        if result.returncode == 0 or diagnostic not in result.stderr:
            raise RuntimeError(name + " did not fail as expected: " + result.stderr)
        print("PASS: linker rejects " + name)

    source = directory / "missing-handlers.c"
    source.write_text("void SystemInit(void) {}\nvoid __libc_init_array(void) {}\n"
                      "int main(void) { for (;;) {} }\n", encoding="utf-8")
    result = subprocess.run([args.compiler, "-mcpu=cortex-m3", "-mthumb", "-nostdlib",
                             "-Wl,-z,noexecstack", "-T" + str(linker), str(source),
                             str(ROOT / "bsp/stm32/stm32f103c8/GNU/startup_stm32f103c8.S"),
                             "-o", str(directory / "missing-handlers.elf")],
                            capture_output=True, text=True)
    if result.returncode == 0 or not all(name in result.stderr for name in
                                       ("PendSV_Handler", "SysTick_Handler")):
        raise RuntimeError("Missing critical handlers did not fail the link")
    print("PASS: linker rejects missing PendSV/SysTick handlers")
    print("PASS: 11 image/linker negative checks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
