"""Check the SDCC images and execute the MCS-51 port on uCsim."""

from pathlib import Path
import re
import subprocess
import sys


def symbol(map_text, name):
    match = re.search(rf"(?m)^\s*(?:[A-Z]:\s*)?([0-9A-Fa-f]+)\s+{name}\s", map_text)
    if match is None:
        raise ValueError(f"Missing map symbol {name}")
    return int(match[1], 16)


def inspect_image(path):
    memory = {}
    eof = False
    for line in path.read_text(encoding="ascii").splitlines():
        if eof or not line.startswith(":"):
            raise ValueError(f"{path.name}: invalid Intel HEX record")
        record = bytes.fromhex(line[1:])
        if len(record) < 5 or len(record) != record[0] + 5 or sum(record) & 0xFF:
            raise ValueError(f"{path.name}: invalid Intel HEX length or checksum")
        count = record[0]
        if record[3] == 1:
            if count:
                raise ValueError(f"{path.name}: invalid Intel HEX EOF")
            eof = True
            continue
        if record[3] != 0:
            raise ValueError(f"{path.name}: unsupported Intel HEX address record")
        start = int.from_bytes(record[1:3], "big")
        if start + count > 65536 or any(address in memory for address in range(start, start + count)):
            raise ValueError(f"{path.name}: overlapping data or Flash overflow")
        memory.update((start + index, value) for index, value in enumerate(record[4:-1]))
    if not eof:
        raise ValueError(f"{path.name}: missing Intel HEX EOF")
    map_text = path.with_suffix(".map").read_text(encoding="ascii")
    for address, name in ((0x0000, "__sdcc_gsinit_startup"),
                          (0x000B, "_s_mcs51_switch_isr"),
                          (0x001B, "_stc8h_tick_isr")):
        vector = bytes(memory[address + offset] for offset in range(3))
        if vector[0] != 0x02 or int.from_bytes(vector[1:], "big") != symbol(map_text, name):
            raise ValueError(f"{path.name}: wrong vector at 0x{address:04x}")
    return map_text


def main():
    s51, smoke_path, firmware_path = sys.argv[1:]
    smoke = Path(smoke_path)
    inspect_image(Path(firmware_path))
    map_text = inspect_image(smoke)
    fault = symbol(map_text, "_s_mcs51_stack_fault")
    start = symbol(map_text, "__start__stack")
    result = subprocess.run(
        [s51, "-t", "C52", "-I", "if=xram[0x1fff]", "-c", "-", str(smoke)],
        input=f"break 0x{fault:x}\nrun\nstate\nquit\n",
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15,
        check=True,
    )
    print(result.stdout, end="")
    if not re.search(r"(?m)^P\s*$", result.stdout) or not re.search(
        rf"Stop at 0x0*{fault:x}:.*breakpoint", result.stdout, re.IGNORECASE
    ):
        raise ValueError("Expected scheduler pass and short-stack trap")
    peak = re.search(r"Max value of stack pointer= 0x([0-9A-Fa-f]+)", result.stdout)
    if peak is None:
        raise ValueError("uCsim did not report a stack high-water mark")
    used = int(peak[1], 16) - start + 1
    limit = symbol(map_text, "l_SSEG")
    if not 0 < used <= limit:
        raise ValueError(f"Hardware stack peak {used} exceeds {limit} bytes")
    print(f"PASS: MCS-51 port, peak hardware stack {used}/{limit} bytes")


if __name__ == "__main__":
    try:
        main()
    except (OSError, subprocess.SubprocessError, ValueError, KeyError) as error:
        sys.exit(f"ERROR: {error}")
