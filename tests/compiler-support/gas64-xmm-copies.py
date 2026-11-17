"""Project: FreeBASIC GAS64 optimizer regression
File: gas64-xmm-copies.py
Purpose: Preserve scalar bits through production store/reload rewrites.
Responsibilities: Run the real helper, assemble its output, and check native values.
This file intentionally does NOT substitute a model for the production optimizer.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def run(command, working):
    result = subprocess.run([str(arg) for arg in command], cwd=working,
                            capture_output=True, text=True, timeout=120, check=False)
    if result.returncode:
        raise RuntimeError(" ".join(map(str, command)) + "\n" + result.stdout + result.stderr)
    return result.stdout


def check(args, working):
    root = Path(__file__).resolve().parents[2]
    production = args.source or root / "src/compiler/backend/gas64/ir-gas64.bas"
    text = production.read_text(encoding="utf-8")
    helper = re.search(r"(?ms)^private sub hOptimRewriteFromXmm\b.*?^end sub\s*", text)
    if helper is None:
        raise RuntimeError("production XMM rewrite helper was not found")
    (working / "gas64-xmm-rewrite.bi").write_text(
        "'' Project: FreeBASIC GAS64 optimizer regression\n"
        "'' File: gas64-xmm-rewrite.bi\n"
        "'' Purpose: Compile the current production rewrite in isolation.\n"
        "'' Responsibilities: Preserve the production helper verbatim.\n"
        "'' This file intentionally does NOT contain test-specific rewrite rules.\n" +
        helper.group() + "\n'' end of gas64-xmm-rewrite.bi\n", encoding="utf-8")
    harness = working / "rewrite.exe"
    run([args.fbc.resolve(), "-prefix", args.prefix.resolve(), "-gen", "gcc",
         "-i", working, Path(__file__).with_suffix(".bas"), "-x", harness], working)
    observations = run([harness], working).splitlines()
    expected_cases = [(op, dest) for op in ("movss", "movd", "movsd", "movq")
                      for dest in ("register", "memory")]
    if len(observations) != len(expected_cases):
        raise RuntimeError("incomplete production rewrite observations")
    triple = run([args.gcc, "-dumpmachine"], working).strip().lower()
    native_x64 = triple.split("-", 1)[0] in ("x86_64", "amd64")

    assembly = [".intel_syntax noprefix", ".text"]
    declarations, checks = [], []
    instruction_failures = []
    for index, (line, (store, destination)) in enumerate(zip(observations, expected_cases)):
        fields = line.split("|")
        if len(fields) != 4 or fields[:2] != [store, destination]:
            raise RuntimeError("unexpected production rewrite observation: " + line)
        first, chained = fields[2:]
        # Only these scalar register/store forms belong to this regression.
        for instruction in (first, chained):
            if not re.fullmatch(r"mov(?:ss|sd|aps|d|q) (?:xmm[23]|-16\[rbp\]), xmm0", instruction):
                raise RuntimeError("unexpected replacement instruction: " + instruction)
        wide = store in ("movsd", "movq")
        copy_ops = ("movq", "movaps", "movsd") if wide else ("movss", "movq", "movaps")
        store_ops = ("movq", "movsd") if wide else ("movss", "movd")
        target = "xmm3" if destination == "register" else "-16[rbp]"
        allowed_first = {op + " xmm2, xmm0" for op in copy_ops}
        allowed_chained = {op + " " + target + ", xmm0"
                           for op in (copy_ops if destination == "register" else store_ops)}
        if first not in allowed_first or chained not in allowed_chained:
            instruction_failures.append(line)

        # Each observer receives its value in XMM0 on both Win64 and System V.
        # Start the register at zero and memory at a poison value. A narrowed
        # DOUBLE copy then cannot pass by inheriting coincidentally valid bits.
        for stage, instruction in (("first", first), ("chained", chained)):
            symbol = "gas64_xmm_" + str(index) + "_" + stage
            asm_symbol = ("_" if "darwin" in triple else "") + symbol
            ctype = "double" if wide else "float"
            width = 64 if wide else 32
            declarations.append(f"extern {ctype} {symbol}({ctype});")
            checks.append(f"CHECK({width}, {symbol});")
            assembly += [".globl " + asm_symbol, asm_symbol + ":", "push rbp", "mov rbp, rsp",
                         "sub rsp, 32", "xorps xmm2, xmm2", "xorps xmm3, xmm3",
                         "movabs rax, 0x7ff40000ffffffff", "mov QWORD PTR -16[rbp], rax",
                         store + " -8[rbp], xmm0", instruction]
            result_register = "xmm2" if stage == "first" else "xmm3"
            if stage == "chained" and destination == "memory":
                assembly.append(("movsd" if wide else "movss") + " xmm3, -16[rbp]")
            assembly += ["movaps xmm0, " + result_register, "leave", "ret"]
    if not any(system in triple for system in ("darwin", "mingw", "cygwin")):
        assembly.append('.section .note.GNU-stack,"",@progbits')
    if not native_x64:
        if instruction_failures:
            raise RuntimeError("scalar widths changed: " + "; ".join(instruction_failures))
        print("GAS64_XMM_COPIES_PASS cases=8 stages=2 native-values=unrun target=" + triple)
        return
    (working / "copies.s").write_text("\n".join(assembly) + "\n", encoding="utf-8")
    # Compare representations rather than floating equality, including signed
    # zero and NaN payloads. No arithmetic is performed on the supplied values.
    driver = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static const uint32_t bits32[] = {
    0, UINT32_C(0x80000000), UINT32_C(0x3fc00001), UINT32_C(0xbfc00001),
    1, UINT32_C(0x7f7fffff), UINT32_C(0x7f800000), UINT32_C(0x7fc12345)
};
static const uint64_t bits64[] = {
    0, UINT64_C(0x8000000000000000), UINT64_C(0x3ff8000000000001),
    UINT64_C(0xbff8000000000001), 1, UINT64_C(0x7fefffffffffffff),
    UINT64_C(0x7ff0000000000000), UINT64_C(0x7ff8123456789abc)
};
typedef float scalar32;
typedef double scalar64;
#define CHECK(W, F) do { \
    for (unsigned i = 0; i < sizeof(bits##W) / sizeof(bits##W[0]); ++i) { \
        scalar##W input, output; uint##W##_t actual; \
        memcpy(&input, &bits##W[i], sizeof(input)); \
        output = F(input); memcpy(&actual, &output, sizeof(actual)); \
        ++count; \
        if (actual != bits##W[i]) { \
            fprintf(stderr, "%s lost scalar bits at input %u\n", #F, i); \
            return 1; \
        } \
    } \
} while (0)
'''
    driver += "\n".join(declarations) + "\nint main(void) { unsigned count = 0;\n"
    driver += "\n".join(checks) + '\nprintf("GAS64_XMM_COPY_VALUES_PASS %u\\n", count); return 0; }\n'
    (working / "driver.c").write_text(driver, encoding="utf-8")
    native = working / "native.exe"
    run([args.gcc, "-O2", "-std=c99", "-Wall", "-Wextra", "-Werror",
         working / "driver.c", working / "copies.s", "-o", native], working)
    print(run([native], working).strip())
    if instruction_failures:
        raise RuntimeError("scalar widths changed: " + "; ".join(instruction_failures))
    print("GAS64_XMM_COPIES_PASS cases=8 stages=2")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--gcc", default=os.environ.get("GCC") or "gcc")
    parser.add_argument("--source", type=Path, help="Use an earlier helper for a regression control")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="fbc-gas64-xmm-") as directory:
        check(args, Path(directory))


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        raise SystemExit("GAS64_XMM_COPIES_FAIL " + str(error))

# end of gas64-xmm-copies.py
