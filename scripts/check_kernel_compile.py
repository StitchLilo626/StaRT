#!/usr/bin/env python3
"""Cross-compile Cortex-M3 C objects only; never link or run firmware."""

import argparse
import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
BOARD_CONFIG = ROOT / "bsp/stm32/stm32f103c8/Core/Inc/StaRT_Config.h"
IPC_OFF = {
    "START_USING_SEMAPHORE": 0,
    "START_USING_MUTEX": 0,
    "START_USING_MESSAGEQUEUE": 0,
}
IPC_ON = {"START_USING_IPC": 1, **{name: 1 for name in IPC_OFF}}
# Overlay the real BSP configuration, so new defaults remain visible to CI.
# All eight IPC child combinations are checked with the parent enabled.
PROFILES = {
    "bsp": {},
    "minimal": {
        **IPC_OFF, "START_USING_IPC": 0, "START_USING_CPU_FFS": 0,
        "START_DEBUG": 0, "START_USING_UDELAY": 0,
        "START_IDLE_HOOK_LIST_SIZE": 0, "START_THREAD_PRIORITY_MAX": 8,
        "START_TICK": 100, "S_PRINTF_BUF_SIZE": 32,
    },
    "ipc-empty": {**IPC_OFF, "START_USING_IPC": 1},
    "semaphore-only": {
        **IPC_OFF, "START_USING_IPC": 1, "START_USING_SEMAPHORE": 1,
        "START_IDLE_HOOK_LIST_SIZE": 0,
    },
    "mutex-only": {
        **IPC_OFF, "START_USING_IPC": 1, "START_USING_MUTEX": 1,
        "START_USING_CPU_FFS": 0,
    },
    "messagequeue-only": {
        **IPC_OFF, "START_USING_IPC": 1, "START_USING_MESSAGEQUEUE": 1,
        "START_THREAD_PRIORITY_MAX": 16, "START_USING_UDELAY": 0,
    },
    "no-semaphore": {**IPC_ON, "START_USING_SEMAPHORE": 0, "START_DEBUG": 0},
    "no-mutex": {**IPC_ON, "START_USING_MUTEX": 0},
    "no-messagequeue": {**IPC_ON, "START_USING_MESSAGEQUEUE": 0},
    "ipc-all": {
        **IPC_ON, "START_USING_CPU_FFS": 1, "START_DEBUG": 1,
        "START_USING_UDELAY": 1, "START_IDLE_HOOK_LIST_SIZE": 4,
    },
    # Parent must hide all child types/APIs even if child switches stay on.
    "ipc-parent-off": {**IPC_ON, "START_USING_IPC": 0},
    "limits": {
        "START_THREAD_PRIORITY_MAX": 1, "START_TIMER_SKIP_LIST_LEVEL": 4,
        "START_IDLE_HOOK_LIST_SIZE": 1, "START_TICK": 100,
        "START_IDLE_STACK_SIZE": 128,
    },
}
FLAGS = [
    "-mcpu=cortex-m3", "-mthumb", "-mfloat-abi=soft", "-std=c99",
    "-ffreestanding", "-fno-common", "-ffunction-sections", "-fdata-sections",
    "-Wall", "-Wextra", "-Werror", "-Wstrict-prototypes",
    "-Wmissing-prototypes", "-Wold-style-definition", "-Wundef",
    "-Wshadow", "-Wformat=2",
]


def run(command):
    result = subprocess.run(command, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True,
                            encoding="utf-8", errors="replace", check=False)
    if result.stdout:
        print(result.stdout, end="", flush=True)
    return result


def compile_profile(compiler, profile, optimization, output, verbose):
    directory = output / profile / optimization
    config_dir = directory / "config"
    config_dir.mkdir(parents=True, exist_ok=True)
    wrapper = ['/* Generated compile-check overlay; not a board config. */',
               '#include "{}"'.format(BOARD_CONFIG.as_posix())]
    for macro, value in PROFILES[profile].items():
        wrapper.extend(["#undef " + macro, "#define {} {}".format(macro, value)])
    (config_dir / "StaRT_Config.h").write_text("\n".join(wrapper) + "\n",
                                             encoding="utf-8")

    sources = []
    for folder in ("src", "libcpu/CM3"):
        found = sorted((ROOT / folder).rglob("*.c"))
        if not found:
            raise RuntimeError("No C sources in " + folder)
        sources.extend(found)
    sources.append(ROOT / "scripts/compile_checks/api.c")
    # Include each public header first, twice, with no transitive prerequisites.
    for header in sorted((ROOT / "include").rglob("*.h")):
        relative = header.relative_to(ROOT / "include").as_posix()
        probe = directory / ("header_" + relative.replace("/", "_") + ".c")
        code = '#include "{0}"\n#include "{0}"\n'.format(relative)
        if relative == "sdef.h":
            code += (
                "#if START_USING_IPC && START_USING_MESSAGEQUEUE\n"
                "typedef char pool_size_requires_size_t[\n"
                "    START_MSGQ_POOL_SIZE(4, 2) == 16 ? 1 : -1];\n"
                "#endif\n"
            )
        probe.write_text(code, encoding="utf-8")
        sources.append(probe)

    flags = [*FLAGS, "-" + optimization, "-I" + str(config_dir),
             "-I" + str(ROOT / "include")]
    records = []
    print("[{} / {}] overrides: {}".format(profile, optimization,
                                          PROFILES[profile]), flush=True)
    for number, source in enumerate(sources):
        obj = directory / "{:02d}_{}.o".format(number, source.stem)
        # A failed recompile must not leave a previous successful object behind.
        obj.unlink(missing_ok=True)
        command = [compiler, *flags, "-MMD", "-c", str(source), "-o", str(obj)]
        if verbose:
            print(subprocess.list2cmdline(command), flush=True)
        result = run(command)
        records.append({"source": str(source), "object": str(obj),
                        "command": command, "returncode": result.returncode})
        if result.returncode:
            print("FAIL: " + str(source), flush=True)
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="arm-none-eabi-gcc",
                        help="GNU Arm C compiler executable (not a shell command)")
    parser.add_argument("--profile", action="append", choices=PROFILES,
                        help="repeat to select profiles; default: all")
    parser.add_argument("--optimization", action="append", choices=("O0", "Os"),
                        help="repeat to select optimization levels; default: both")
    parser.add_argument("--output", type=Path, default=ROOT / "build/cortex-m3")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    profiles = list(dict.fromkeys(args.profile or PROFILES))
    optimizations = list(dict.fromkeys(args.optimization or ("O0", "Os")))
    try:
        version = run([args.compiler, "--version"])
        machine = run([args.compiler, "-dumpmachine"])
        if version.returncode or machine.returncode or machine.stdout.strip() != "arm-none-eabi":
            raise RuntimeError("Expected an arm-none-eabi cross-compiler")
        records = []
        for profile in profiles:
            for optimization in optimizations:
                records.extend(compile_profile(args.compiler, profile, optimization,
                                               output, args.verbose))
        failures = sum(record["returncode"] != 0 for record in records)
        report = {
            "scope": "Cortex-M3 C objects only; no assembly, link or execution",
            "compiler": version.stdout.strip(), "flags": FLAGS,
            "profiles": {name: PROFILES[name] for name in profiles},
            "optimizations": optimizations, "failures": failures,
            "compilations": records,
        }
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n",
                                           encoding="utf-8")
        print("{}: {} C compilations, {} failure(s), {} configurations x {} optimization(s)."
              .format("FAIL" if failures else "PASS", len(records), failures,
                      len(profiles), len(optimizations)))
        print("C objects only: assembly, firmware linking and hardware are NOT checked.")
        print("Report: " + str(output / "report.json"))
        return 1 if failures else 0
    except (OSError, RuntimeError) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
