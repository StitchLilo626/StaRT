#!/usr/bin/env python3
"""Validate a linked STM32F103C8 ELF and its BIN/HEX exports (no execution)."""

import argparse
import json
from pathlib import Path
import struct
import sys


FLASH = 0x08000000
FLASH_SIZE = 64 * 1024
RAM = 0x20000000
RAM_SIZE = 20 * 1024


def require(condition, message):
    if not condition:
        raise ValueError(message)


class Elf:
    def __init__(self, data):
        self.data = data
        require(data[:7] == b"\x7fELF\x01\x01\x01", "Expected little-endian ELF32")
        header = self.unpack("16sHHIIIIIHHHHHH", 0)
        require(header[1:3] == (2, 40), "Expected linked ARM executable")
        self.entry = header[4]
        require(header[7] & 0xff000000 == 0x05000000, "Expected ARM EABI5")
        require(not header[7] & 0x400, "Hard-float ABI is invalid on Cortex-M3")
        require(header[9] == 32 and header[11] == 40, "Unexpected ELF table layout")
        self.segments = [self.unpack("8I", header[5] + i * 32) for i in range(header[10])]
        sections = [self.unpack("10I", header[6] + i * 40) for i in range(header[12])]
        require(header[13] < len(sections), "Missing section names")
        names = self.section_data(sections[header[13]])
        self.sections = {self.string(names, s[0]): s for s in sections}
        self.symbols = {}
        for section in sections:
            if section[1] != 2:  # SHT_SYMTAB
                continue
            require(section[9] == 16 and section[6] < len(sections), "Invalid symbol table")
            strings = self.section_data(sections[section[6]])
            for offset in range(section[4], section[4] + section[5], 16):
                name, value, size, info, other, index = self.unpack("IIIBBH", offset)
                if name:
                    self.symbols[self.string(strings, name)] = (value, size, info, index)

    def unpack(self, fmt, offset):
        size = struct.calcsize("<" + fmt)
        require(0 <= offset <= len(self.data) - size, "Truncated ELF table")
        return struct.unpack_from("<" + fmt, self.data, offset)

    def section_data(self, section):
        require(section[4] + section[5] <= len(self.data), "Truncated ELF section")
        return self.data[section[4]:section[4] + section[5]]

    @staticmethod
    def string(table, offset):
        require(offset < len(table) and b"\0" in table[offset:], "Invalid ELF string")
        return table[offset:table.index(b"\0", offset)].decode("ascii")

    def symbol(self, name):
        require(name in self.symbols, "Missing symbol: " + name)
        return self.symbols[name][0]


def validate_elf(data):
    elf = Elf(data)
    undefined = [name for name, symbol in elf.symbols.items() if symbol[3] == 0]
    require(not undefined, "Undefined symbols: " + ", ".join(undefined))
    for name in ("Reset_Handler", "PendSV_Handler", "SysTick_Handler", "main",
                 "SystemInit", "__libc_init_array", "_init", "s_start_init",
                 "s_sched_start", "s_irq_disable", "s_irq_enable",
                 "s_first_switch_task", "s_normal_switch_task", "s_putc",
                 "HAL_Init", "HAL_UART_Transmit", "DWT_Delay_us"):
        value = elf.symbol(name)
        require(FLASH <= (value & ~1) < FLASH + FLASH_SIZE and value & 1,
                "Invalid Thumb function: " + name)
        require(elf.symbols[name][2] >> 4 == 1, "Expected strong definition: " + name)

    require(".isr_vector" in elf.sections, "Missing vector section")
    vectors = elf.sections[".isr_vector"]
    require(vectors[3] == FLASH and vectors[5] == 59 * 4, "Invalid vector location/size")
    words = struct.unpack("<59I", elf.section_data(vectors))
    require(words[0] == RAM + RAM_SIZE and words[0] % 8 == 0, "Invalid initial MSP")
    require(elf.entry == words[1] == elf.symbol("Reset_Handler"), "Invalid reset entry")
    for slot, name in ((14, "PendSV_Handler"), (15, "SysTick_Handler")):
        require(words[slot] == elf.symbol(name), "Incorrect vector: " + name)
        require(words[slot] != elf.symbol("Default_Handler"), "Default RTOS interrupt handler")
    for slot, value in enumerate(words[1:], 1):
        if slot in (7, 8, 9, 10, 13):
            require(value == 0, "Reserved vector is not zero")
        else:
            require(value & 1 and FLASH <= (value & ~1) < FLASH + FLASH_SIZE,
                    "Invalid interrupt vector {}".format(slot))

    require(elf.symbol("_estack") == RAM + RAM_SIZE, "Incorrect stack top")
    require(elf.symbol("__StackTop") == elf.symbol("_estack"), "Stack top mismatch")
    require(elf.symbol("__StackLimit") % 8 == 0, "Unaligned MSP stack limit")
    require(RAM <= elf.symbol("_end") <= elf.symbol("__StackLimit"), "RAM/stack overlap")
    for section_name, start, end, section_type in (
        (".data", "_sdata", "_edata", 1), (".bss", "_sbss", "_ebss", 8)
    ):
        section = elf.sections[section_name]
        require(section[1] == section_type and RAM <= section[3] < RAM + RAM_SIZE,
                "Invalid section: " + section_name)
        require(section[3] == elf.symbol(start) and
                section[3] + section[5] == elf.symbol(end), "Startup boundary mismatch")
        require(elf.symbol(start) % 4 == 0 and elf.symbol(end) % 4 == 0,
                "Unaligned startup copy/zero boundaries")

    loads = []
    for kind, offset, vaddr, paddr, filesz, memsz, flags, align in elf.segments:
        if kind != 1:  # PT_LOAD
            continue
        require(filesz <= memsz and offset + filesz <= len(data), "Invalid load segment")
        require(not (flags & 1 and flags & 2), "Writable executable load segment")
        require((FLASH <= vaddr and vaddr + memsz <= FLASH + FLASH_SIZE) or
                (RAM <= vaddr and vaddr + memsz <= RAM + RAM_SIZE), "Load segment outside device")
        if filesz:
            require(FLASH <= paddr and paddr + filesz <= FLASH + FLASH_SIZE,
                    "Flash load image overflow")
            loads.append((paddr, data[offset:offset + filesz]))
            if vaddr == elf.symbol("_sdata"):
                require(paddr == elf.symbol("_sidata"), "Incorrect .data load address")
    require(loads and min(address for address, _ in loads) == FLASH, "Missing Flash image")
    flash_size = max(address + len(chunk) for address, chunk in loads) - FLASH
    image = bytearray(flash_size)
    for address, chunk in loads:
        image[address - FLASH:address - FLASH + len(chunk)] = chunk
    return {
        "flash_bytes": flash_size,
        "static_ram_bytes": elf.symbol("_end") - RAM,
        "reserved_msp_bytes": elf.symbol("__StackTop") - elf.symbol("__StackLimit"),
        "free_ram_bytes": elf.symbol("__StackLimit") - elf.symbol("_end"),
        "entry": hex(elf.entry), "vectors": len(words), "undefined_symbols": undefined,
    }, bytes(image)


def validate_hex(text, expected, entry):
    image = bytearray(len(expected))
    base = 0
    eof = False
    addresses = set()
    for line in text.splitlines():
        require(not eof and line.startswith(":"), "Invalid Intel HEX record")
        record = bytes.fromhex(line[1:])
        require(len(record) >= 5 and len(record) == record[0] + 5 and sum(record) % 256 == 0,
                "Intel HEX length/checksum mismatch")
        size, high, low, kind = record[:4]
        payload = record[4:-1]
        address = (high << 8) | low
        if kind == 0:
            start = base + address - FLASH
            require(0 <= start and start + size <= len(expected), "HEX data outside Flash image")
            require(not addresses.intersection(range(start, start + size)), "Overlapping HEX data")
            addresses.update(range(start, start + size))
            image[start:start + size] = payload
        elif kind == 4:
            require(size == 2, "Invalid HEX extended address")
            base = int.from_bytes(payload, "big") << 16
        elif kind == 5:
            require(size == 4 and int.from_bytes(payload, "big") == entry, "Incorrect HEX entry")
        elif kind == 1:
            require(size == 0, "Invalid HEX EOF")
            eof = True
        else:
            raise ValueError("Unexpected Intel HEX record type")
    require(eof and addresses and min(addresses) == 0 and max(addresses) == len(expected) - 1,
            "Incomplete HEX image")
    require(bytes(image) == expected, "HEX does not match ELF Flash image")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    try:
        report, expected = validate_elf(args.elf.read_bytes())
        require(args.elf.with_suffix(".bin").read_bytes() == expected, "BIN does not match ELF")
        validate_hex(args.elf.with_suffix(".hex").read_text(encoding="ascii"),
                     expected, int(report["entry"], 16))
        require(args.elf.with_suffix(".map").stat().st_size > 0, "Missing linker map")
        if args.report:
            args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print("PASS: full STM32F103C8 firmware link and ELF/BIN/HEX checks")
        print("Flash {flash_bytes}/65536 B; static RAM {static_ram_bytes} B; "
              "MSP reserve {reserved_msp_bytes} B; unallocated RAM {free_ram_bytes} B".format(**report))
        print("Build/image validation only; STM32 peripheral timing and hardware were not tested.")
        return 0
    except (OSError, ValueError, KeyError, struct.error) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
