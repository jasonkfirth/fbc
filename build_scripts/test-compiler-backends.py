#!/usr/bin/env python3
"""FreeBASIC compiler native backend regression runner.

File: test-compiler-backends.py
Purpose: Check external ABI names, inline assembly, and builtin declarations.
Responsibilities: Compile and execute focused fixtures with available backends.
This file intentionally does NOT replace the full language or corpus suites.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--toolchain-prefix", type=Path,
                        help="Use runtime libraries from a separate native build")
    parser.add_argument("--backend", choices=("gcc", "llvm", "clang"), action="append")
    options = parser.parse_args()
    root = options.root.resolve()
    toolchain_prefix = (options.toolchain_prefix or root).resolve()
    compiler_args = [str(options.fbc.resolve()), "-prefix", str(toolchain_prefix)]
    backends = options.backend or ["gcc"]
    clang = shutil.which(os.environ.get("CLANG") or "clang")
    llc = shutil.which(os.environ.get("LLC") or "llc")
    if options.backend is None:
        if llc:
            backends.append("llvm")
        if clang:
            backends.append("clang")
    fixtures = [
        ("abi", "tests/compiler-support/backend-abi.bas", "backend ABI passed"),
        ("asm-att-llvm", "tests/quirk/inline-asm-att-llvm.bas",
         "PASS: inline assembly dialect"),
        ("builtins", "tests/crt/builtin-backends.bas", ""),
        ("asm-registers", "tests/quirk/inline-asm-registers.bas", ""),
        ("qb-headers", "tests/qb/vb-string-helpers.bas", ""),
    ]
    with tempfile.TemporaryDirectory(prefix="fbc-backends-") as temporary:
        working = Path(temporary)
        # Every compiler contains GAS64, even when its host has 32-bit INTEGER.
        # Emission checks that host-width references match the fixed-width state
        # without requiring the target's assembler, linker, or runtime libraries.
        for target in ("win32", "linux-mips32", "riscos"):
            output = working / ("ir-gas64-" + target + ".c")
            command = [*compiler_args, "-r",
                       "-gen", "gcc", "-target", target, "-m", "fbc",
                       "-i", str(root / "src/compiler"), "-i", str(root / "inc"),
                       str(root / "src/compiler/backend/gas64/ir-gas64.bas"),
                       "-o", str(output)]
            emitted = subprocess.run(command, cwd=working, text=True,
                                     capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print(f"GAS64/{target}: emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1
            print(f"GAS64/{target}: emission passed")
            # Later checks do not consume these large cross-target emissions.
            # Close each passed fixture before producing the next one.
            output.unlink()
        # GAS32 sees pointer-index operands without a backing symbol while
        # compiling this module. TLS operand checks must accept those operands.
        command = [*compiler_args, "-r",
                   "-gen", "gas", "-target", "win32", "-m", "fbc", "-e",
                   "-i", str(root / "src/compiler"), "-i", str(root / "inc"),
                   str(root / "src/compiler/backend/gas64/ir-gas64.bas"),
                   "-o", str(working / "ir-gas64-win32.asm")]
        emitted = subprocess.run(command, cwd=working, text=True,
                                 capture_output=True, timeout=120, check=False)
        if emitted.returncode:
            print(f"GAS32/GAS64: emission failed\n{emitted.stdout}{emitted.stderr}")
            return 1
        print("GAS32/GAS64: emission passed")
        # CE's PE object format cannot encode the 16-byte storage alignment
        # used by desktop targets. Compile the language suite's real wide
        # integer globals and layout checks through each CE C target.
        for target in ("wince-arm", "wince-mips"):
            output = working / ("integer128-" + target + ".c")
            command = [*compiler_args, "-r",
                       "-gen", "gcc", "-target", target,
                       "-i", str(root / "inc/wince"), "-i", str(root / "inc"),
                       "-i", str(root / "tests/fbcunit/inc"),
                       str(root / "tests/numbers/integer128.bas"), "-o", str(output)]
            emitted = subprocess.run(command, cwd=working, text=True,
                                     capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print(f"Int128/{target}: emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1
            generated = output.read_text(encoding="utf-8")
            if not re.search(r"typedef struct __attribute__\(\(aligned\(8\)\)\).*FB_INT128;", generated):
                print(f"Int128/{target}: incorrect storage alignment")
                return 1
            print(f"Int128/{target}: storage alignment passed")
        # Exercise every supported OS, including both common desktop widths
        # and CE architectures. Target overrides precede the shared CRT tree,
        # matching the installed compiler's include order.
        crt_targets = (
            "win32", "win64", "win32-aarch64", "wince-arm", "wince-mips",
            "cygwin", "linux", "android", "haiku", "dos", "xbox", "freebsd",
            "dragonfly", "solaris", "illumos", "openbsd", "darwin", "netbsd",
            "js-asmjs", "wii", "nuttx", "riscos", "aros", "amiga",
            "freebsd-x86", "netbsd-x86", "openbsd-x86", "aros-x86",
            "haiku-x86", "darwin-aarch64", "linux-aarch64",
        )
        for target in crt_targets:
            target_headers = root / "inc" / target.split("-", 1)[0]
            command = [*compiler_args, "-r",
                       "-gen", "gcc", "-target", target]
            if target_headers.is_dir():
                command += ["-i", str(target_headers)]
            command += ["-i", str(root / "inc"),
                        str(root / "tests/crt/platform-headers.bas"),
                        "-o", str(working / ("crt-" + target + ".c"))]
            emitted = subprocess.run(command, cwd=working, text=True,
                                     capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print(f"CRT/{target}: emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1
            if target.startswith("wince-"):
                # The CE SDK implements GetCurrentProcessId inline. The runtime
                # bridge supplies an ordinary C export for BASIC callers.
                generated = (working / ("crt-" + target + ".c")).read_text(encoding="utf-8")
                if "fb_hWinCEGetProcessId" not in generated or re.search(r"\b(getpid|GetCurrentProcessId)\b", generated):
                    print(f"CRT/{target}: incorrect process-ID export")
                    return 1
            print(f"CRT/{target}: emission passed")
        # RISC OS also overrides fbnetwire.bi for its APCS double layout.
        # Compile the actual Unicode fixture on the previously failing targets.
        for target in ("riscos", "freebsd", "netbsd", "openbsd", "aros", "amiga", "wince-arm"):
            target_headers = root / "inc" / target.split("-", 1)[0]
            command = [*compiler_args, "-r",
                       "-gen", "gcc", "-target", target]
            if target_headers.is_dir():
                command += ["-i", str(target_headers)]
            command += ["-i", str(root / "inc"), "-i", str(root / "tests/fbcunit/inc"),
                        str(root / "tests/string/text-types.bas"),
                        "-o", str(working / ("text-types-" + target + ".c"))]
            emitted = subprocess.run(command, cwd=working, text=True,
                                     capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print(f"Unicode/{target}: emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1
            print(f"Unicode/{target}: emission passed")
        if "llvm" in backends:
            if llc is None:
                print("llvm: configured compiler is unavailable")
                return 1
            # Run llc for ARM64 even on an x86 host. IR-only checks would miss
            # an architecture name copied from the GCC command-line interface.
            source = working / "llvm-aarch64.bas"
            source.write_text('function increment( byval value as longint ) as longint\n'
                              '    return value + 1\nend function\n', encoding="utf-8")
            command = [*compiler_args, "-rr",
                       "-gen", "llvm", "-target", "win32", "-arch", "aarch64",
                       str(source)]
            try:
                # Explicit targets select prefixed tools. This emission check
                # uses the detected host llc, which contains the ARM64 backend.
                emitted = subprocess.run(command, cwd=working, text=True,
                                         capture_output=True, timeout=120, check=False,
                                         env={**os.environ, "LLC": llc})
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"LLVM/aarch64: {error}")
                return 1
            assembly = source.with_suffix(".asm")
            if emitted.returncode or not assembly.is_file() or assembly.stat().st_size == 0:
                print(f"LLVM/aarch64: assembly emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1
            print("LLVM/aarch64: assembly emission passed")
            if clang is not None:
                # Assemble both Windows x86 targets with the detected host
                # Clang. Its default CPU must not decide the object's word size.
                # The first COFF header field identifies i386 (0x14c) or AMD64
                # (0x8664), independently of the compiler host architecture.
                for architecture, machine in (("686", 0x14C), ("x86_64", 0x8664)):
                    obj = working / ("llvm-win32-" + architecture + ".o")
                    command = [*compiler_args, "-c",
                               "-gen", "llvm", "-target", "win32", "-arch", architecture,
                               str(source), "-o", str(obj)]
                    try:
                        compiled = subprocess.run(command, cwd=working, text=True,
                                                  capture_output=True, timeout=120, check=False,
                                                  env={**os.environ, "LLC": llc, "CLANG": clang})
                    except (OSError, subprocess.TimeoutExpired) as error:
                        print(f"LLVM/win32-{architecture}: {error}")
                        return 1
                    if (compiled.returncode or not obj.is_file() or
                            int.from_bytes(obj.read_bytes()[:2], "little") != machine):
                        print(f"LLVM/win32-{architecture}: object compilation failed\n"
                              f"{compiled.stdout}{compiled.stderr}")
                        return 1
                    print(f"LLVM/win32-{architecture}: object compilation passed")
        try:
            target = subprocess.run([str(options.fbc.resolve()), "-print", "target"],
                                    cwd=working, text=True, capture_output=True,
                                    timeout=30, check=False)
            if target.returncode:
                print(f"compiler target query failed\n{target.stdout}{target.stderr}")
                return 1
            system, _, architecture = target.stdout.strip().partition("-")
            int128_object = None
            int128_input_object = None
            thread_local_extern_object = None
            incomplete_extern_object = None
            if architecture == "x86_64" and system in (
                    "linux", "android", "darwin", "freebsd", "openbsd", "netbsd",
                    "dragonfly", "solaris", "illumos", "haiku"):
                c_object = working / "backend-c-abi.o"
                c_build = subprocess.run([os.environ.get("GCC") or "gcc", "-m64", "-c",
                                          str(root / "tests/compiler-support/backend-c-abi.c"),
                                          "-o", str(c_object)], cwd=working, text=True,
                                         capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print(f"C ABI fixture compilation failed\n{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.extend((
                    ("c-abi", "tests/compiler-support/backend-c-abi.bas", "C aggregate ABI passed"),
                    ("c-abi-opt", "tests/compiler-support/backend-c-abi.bas", "C aggregate ABI passed"),
                ))
                int128_object = working / "backend-int128-abi.o"
                c_build = subprocess.run([os.environ.get("GCC") or "gcc", "-m64", "-c",
                                          str(root / "tests/compiler-support/integer128-abi.c"),
                                          "-o", str(int128_object)], cwd=working, text=True,
                                         capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print(f"128-bit C ABI fixture compilation failed\n{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.append(("int128-abi", "tests/compiler-support/integer128-abi.bas",
                                 "128-bit C ABI passed"))
            if architecture == "x86_64" and system == "linux":
                fixtures.append(("cva-forward-abi",
                                 "tests/compiler-support/cva-forward-abi.bas",
                                 "C va_list forwarding passed"))
                int128_input_object = working / "integer128-abi-input.o"
                c_build = subprocess.run([os.environ.get("GCC") or "gcc", "-m64", "-c",
                                          str(root / "tests/compiler-support/integer128-abi-input.c"),
                                          "-o", str(int128_input_object)], cwd=working, text=True,
                                         capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print(f"128-bit argument ABI fixture compilation failed\n{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.append(("int128-abi-input", "tests/compiler-support/integer128-abi-input.bas",
                                 "128-bit argument ABI passed"))
                thread_local_extern_object = working / "thread-local-extern.o"
                c_build = subprocess.run([
                    os.environ.get("GCC") or "gcc", "-c",
                    str(root / "tests/compiler-support/thread-local-extern.c"),
                    "-o", str(thread_local_extern_object)], cwd=working, text=True,
                    capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print("thread-local external fixture compilation failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.append(("thread-local-extern",
                                 "tests/compiler-support/thread-local-extern.bas",
                                 "thread-local external passed"))
                incomplete_extern_object = working / "incomplete-extern.o"
                c_build = subprocess.run(
                    [os.environ.get("GCC") or "gcc", "-m64", "-c",
                     str(root / "tests/compiler-support/incomplete-extern.c"),
                     "-o", str(incomplete_extern_object)], cwd=working, text=True,
                    capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print("incomplete external fixture compilation failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.append(("incomplete-extern",
                                 "tests/compiler-support/incomplete-extern.bas", ""))
            if architecture in ("x86_64", "aarch64"):
                fixtures.extend((
                    ("int128-layout", "tests/compiler-support/integer128-layout.bas",
                     "128-bit integer layout passed"),
                    ("int128-arithmetic", "tests/compiler-support/integer128-arithmetic.bas",
                     "128-bit integer arithmetic passed"),
                    ("thread-local", "tests/compiler-support/thread-local.bas",
                     "thread-local storage passed"),
                ))
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"C ABI fixture: {error}")
            return 1
        for backend in backends:
            for name, source, expected in fixtures:
                if name == "asm-att-llvm" and backend != "llvm":
                    continue
                executable = working / (backend + "-" + name)
                input_source = root / source
                if name.startswith("c-abi"):
                    input_source = working / (backend + "-" + name + ".bas")
                    shutil.copy2(root / source, input_source)
                # The suite runs from a temporary directory, so resolve sibling
                # includes from the fixture's source directory explicitly.
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-i", str((root / source).parent),
                           "-gen", backend, "-v",
                           str(input_source), "-x", str(executable)]
                if system == "darwin" and architecture in ("x86", "x86_64") and name in (
                        "abi", "asm-registers") and backend in ("gcc", "clang"):
                    # These fixtures contain Intel tokens. Darwin defaults to
                    # AT&T string expressions, so select their actual syntax.
                    command += ["-asm", "intel"]
                if name == "asm-att-llvm":
                    command += ["-asm", "att"]
                if name == "qb-headers":
                    command += ["-lang", "qb"]
                if name.startswith("thread-local"):
                    command += ["-mt"]
                if name == "thread-local-extern":
                    command += [str(thread_local_extern_object)]
                if name == "incomplete-extern":
                    command += [str(incomplete_extern_object)]
                if name.startswith("c-abi"):
                    command += [str(c_object)]
                    if backend == "llvm":
                        command += ["-R"]
                if name == "int128-abi":
                    command += [str(int128_object)]
                if name == "int128-abi-input":
                    command += [str(int128_input_object)]
                if name.endswith("-opt"):
                    command += ["-O", "3"]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print(f"{backend}/{name}: compilation failed: {command!r}\n"
                              f"{built.stdout}{built.stderr}")
                        return 1
                    if backend == "llvm" and name.startswith("c-abi"):
                        # byval's alignment applies to the source pointer too.
                        # x86 tolerates many unaligned reads, so execution alone
                        # cannot establish the validity of that IR contract.
                        files = list(working.glob(input_source.stem + "*.ll"))
                        if not files:
                            print(f"{backend}/{name}: retained LLVM IR missing")
                            return 1
                        ir = files[0].read_text(encoding="utf-8")
                        slots = {match[0] for match in re.findall(
                            r"^\s*(%[\w.$-]+) = alloca [^\n]+, align (\d+)$", ir, re.M)
                                 if int(match[1]) >= 8}
                        arguments = []
                        for line in ir.splitlines():
                            if "call " in line:
                                arguments.extend(re.findall(
                                    r"byval\([^)]*\) align \d+ (%[\w.$-]+)", line))
                        if not arguments or any(arg not in slots for arg in arguments):
                            print(f"{backend}/{name}: byval argument lacks aligned local storage")
                            return 1
                    if backend == "clang":
                        # This must follow -Wall so Clang's expensive C-only
                        # warning analysis stays disabled for generated code.
                        diagnostic = built.stdout + built.stderr
                        enabled = diagnostic.find("-Wall")
                        disabled = diagnostic.find("-Wno-uninitialized")
                        if enabled < 0 or disabled < enabled:
                            print(f"{backend}/{name}: uninitialized-warning policy missing\n{diagnostic}")
                            return 1
                    executed = subprocess.run([str(executable)], cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"{backend}/{name}: {error}")
                    return 1
                if executed.returncode or executed.stdout.strip() != expected:
                    print(f"{backend}/{name}: execution failed\n{executed.stdout}{executed.stderr}")
                    return 1
                print(f"{backend}/{name}: passed")

        invalid_source = root / "tests/compiler-support/incomplete-extern-invalid-value.bas"
        invalid_output = working / "incomplete-extern-invalid-value.o"
        invalid_build = subprocess.run(
            [*compiler_args, "-c",
             str(invalid_source), "-o", str(invalid_output)], cwd=working,
            text=True, capture_output=True, timeout=60, check=False)
        invalid_diagnostic = invalid_build.stdout + invalid_build.stderr
        if invalid_build.returncode == 0 or "Incomplete type" not in invalid_diagnostic:
            print("incomplete external value operations were not rejected\n"
                  f"{invalid_diagnostic}")
            return 1
        print("incomplete external value operations: rejected")

        if system == "linux" and architecture == "x86_64":
            source = root / "tests/compiler-support/integer128-c-i386.bas"
            gcc = os.environ.get("GCC") or "gcc"
            for backend, driver in (("gcc", gcc), ("clang", clang)):
                if driver is None:
                    continue
                generated = working / ("integer128-c-i386-" + backend + ".c")
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-gen", backend,
                           "-target", "linux", "-arch", "686", "-r",
                           str(source), "-o", str(generated)]
                try:
                    emitted = subprocess.run(command, cwd=working, text=True,
                                             capture_output=True, timeout=120, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"{backend}/i386 int128: {error}")
                    return 1
                if emitted.returncode:
                    print(f"{backend}/i386 int128 emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1

                runtime_test = working / ("integer128-runtime-" + backend)
                # The generated constructor runs the FreeBASIC ABI checks
                # before the helper test's main() checks the arithmetic.
                c_build = subprocess.run(
                    [driver, "-m32", "-std=c99", "-Wall", "-Wextra",
                     "-Werror=implicit-function-declaration", str(generated),
                     str(root / "tests/compiler-support/integer128-runtime.c"),
                     str(root / "src/rtlib/math/int128.c"), "-o", str(runtime_test)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print(f"{backend}/i386 integer128 program compilation failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                executed = subprocess.run([str(runtime_test)], cwd=working, text=True,
                                          capture_output=True, timeout=30, check=False)
                if executed.returncode:
                    print(f"{backend}/i386 integer128 program failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print(f"{backend}/i386 int128 lowering passed")

            runtime_test = working / "integer128-runtime-native-test"
            c_build = subprocess.run(
                [gcc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                 str(root / "tests/compiler-support/integer128-runtime.c"),
                 str(root / "src/rtlib/math/int128.c"), "-o", str(runtime_test)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if c_build.returncode:
                print(f"128-bit runtime test compilation failed\n{c_build.stdout}{c_build.stderr}")
                return 1
            executed = subprocess.run([str(runtime_test)], cwd=working, text=True,
                                      capture_output=True, timeout=30, check=False)
            if executed.returncode:
                print(f"128-bit runtime test failed\n{executed.stdout}{executed.stderr}")
                return 1

            gas_source = root / "tests/compiler-support/integer128-gas32.bas"
            gas_assembly = working / "integer128-gas32-runtime.asm"
            gas_emit_command = [*compiler_args,
                                "-i", str(root / "inc"), "-target", "linux-x86",
                                "-gen", "gas", "-r", str(gas_source),
                                "-o", str(gas_assembly)]
            emitted = subprocess.run(gas_emit_command, cwd=working, text=True,
                                     capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print(f"gas32 int128 emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1

            gas_object = working / "integer128-gas32-runtime.o"
            assembled = subprocess.run(
                [os.environ.get("AS") or "as", "--32", "-o", str(gas_object),
                 str(gas_assembly)], cwd=working, text=True,
                capture_output=True, timeout=60, check=False)
            if assembled.returncode:
                print(f"gas32 int128 assembly failed\n{assembled.stdout}{assembled.stderr}")
                return 1

            gas_runtime = working / "integer128-gas32-runtime"
            c_build = subprocess.run(
                [gcc, "-m32", "-no-pie", "-std=c99", "-Wall", "-Wextra", "-Werror",
                 str(gas_object),
                 str(root / "tests/compiler-support/integer128-gas32-main.c"),
                 str(root / "src/rtlib/math/int128.c"), "-o", str(gas_runtime)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if c_build.returncode:
                print(f"gas32 int128 runtime link failed\n{c_build.stdout}{c_build.stderr}")
                return 1
            executed = subprocess.run([str(gas_runtime)], cwd=working, text=True,
                                      capture_output=True, timeout=30, check=False)
            if executed.returncode:
                print(f"gas32 int128 runtime failed\n{executed.stdout}{executed.stderr}")
                return 1
            print("gas32 int128 runtime passed")

            incomplete_source = root / "tests/compiler-support/incomplete-extern.bas"
            incomplete_support = root / "tests/compiler-support/incomplete-extern.c"
            incomplete_main = root / "tests/compiler-support/integer128-gas32-main.c"
            incomplete_support_object = working / "incomplete-extern-gas32-support.o"
            c_build = subprocess.run(
                [gcc, "-m32", "-Wall", "-Wextra", "-Werror", "-c",
                 str(incomplete_support), "-o", str(incomplete_support_object)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if c_build.returncode:
                print("gas32 incomplete external support compilation failed\n"
                      f"{c_build.stdout}{c_build.stderr}")
                return 1

            incomplete_gas_assembly = working / "incomplete-extern-gas32.asm"
            emitted = subprocess.run(
                [*compiler_args, "-i",
                 str(root / "inc"), "-target", "linux-x86", "-gen", "gas",
                 "-r", str(incomplete_source), "-o", str(incomplete_gas_assembly)],
                cwd=working, text=True, capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print("gas32/incomplete-extern emission failed\n"
                      f"{emitted.stdout}{emitted.stderr}")
                return 1
            incomplete_gas_object = working / "incomplete-extern-gas32.o"
            assembled = subprocess.run(
                [os.environ.get("AS") or "as", "--32", "-o",
                 str(incomplete_gas_object), str(incomplete_gas_assembly)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if assembled.returncode:
                print("gas32/incomplete-extern assembly failed\n"
                      f"{assembled.stdout}{assembled.stderr}")
                return 1
            incomplete_gas_runtime = working / "incomplete-extern-gas32"
            c_build = subprocess.run(
                [gcc, "-m32", "-no-pie", "-Wall", "-Wextra", "-Werror",
                 str(incomplete_gas_object), str(incomplete_support_object),
                 str(incomplete_main), "-o", str(incomplete_gas_runtime)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if c_build.returncode:
                print("gas32/incomplete-extern link failed\n"
                      f"{c_build.stdout}{c_build.stderr}")
                return 1
            executed = subprocess.run([str(incomplete_gas_runtime)], cwd=working,
                                      text=True, capture_output=True, timeout=30, check=False)
            if executed.returncode or executed.stdout.strip():
                print("gas32/incomplete-extern runtime failed\n"
                      f"{executed.stdout}{executed.stderr}")
                return 1
            print("gas32/incomplete-extern passed")

            for backend, driver in (("gcc", gcc), ("clang", clang)):
                if driver is None:
                    continue
                generated = working / ("incomplete-extern-i386-" + backend + ".c")
                emitted = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "linux", "-arch", "686",
                     "-gen", backend, "-r", str(incomplete_source), "-o",
                     str(generated)], cwd=working, text=True,
                    capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print(f"{backend}/i386 incomplete-extern emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1
                runtime_test = working / ("incomplete-extern-i386-" + backend)
                c_build = subprocess.run(
                    [driver, "-m32", "-no-pie", "-Wall", "-Wextra",
                     "-Werror=implicit-function-declaration", str(generated),
                     str(incomplete_support_object), str(incomplete_main),
                     "-o", str(runtime_test)], cwd=working, text=True,
                    capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print(f"{backend}/i386 incomplete-extern link failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                executed = subprocess.run([str(runtime_test)], cwd=working,
                                          text=True, capture_output=True,
                                          timeout=30, check=False)
                if executed.returncode or executed.stdout.strip():
                    print(f"{backend}/i386 incomplete-extern runtime failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print(f"{backend}/i386 incomplete-extern passed")

            if llc is not None:
                incomplete_ir = working / "incomplete-extern-llvm-i386.ll"
                emitted = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "linux-x86", "-gen", "llvm",
                     "-r", str(incomplete_source), "-o", str(incomplete_ir)],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print("llvm/i386 incomplete-extern emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1
                incomplete_llvm_object = working / "incomplete-extern-llvm-i386.o"
                lowered = subprocess.run(
                    [llc, "-mtriple=i386-pc-linux-gnu", "-filetype=obj",
                     str(incomplete_ir), "-o", str(incomplete_llvm_object)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if lowered.returncode:
                    print("llvm/i386 incomplete-extern lowering failed\n"
                          f"{lowered.stdout}{lowered.stderr}")
                    return 1
                incomplete_llvm_runtime = working / "incomplete-extern-llvm-i386"
                c_build = subprocess.run(
                    [gcc, "-m32", "-no-pie", "-Wall", "-Wextra",
                     "-Werror=implicit-function-declaration",
                     str(incomplete_llvm_object), str(incomplete_support_object),
                     str(incomplete_main), "-o", str(incomplete_llvm_runtime)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print("llvm/i386 incomplete-extern link failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                executed = subprocess.run([str(incomplete_llvm_runtime)], cwd=working,
                                          text=True, capture_output=True,
                                          timeout=30, check=False)
                if executed.returncode or executed.stdout.strip():
                    print("llvm/i386 incomplete-extern runtime failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print("llvm/i386 incomplete-extern passed")

            gas_c_abi_source = root / "tests/compiler-support/integer128-gas32-c-abi.bas"
            gas_c_abi_assembly = working / "integer128-gas32-c-abi.asm"
            emitted = subprocess.run(
                [*compiler_args, "-i",
                 str(root / "inc"), "-target", "linux-x86", "-gen", "gas",
                 "-r", str(gas_c_abi_source), "-o", str(gas_c_abi_assembly)],
                cwd=working, text=True, capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print("gas32/C int128 ABI emission failed\n"
                      f"{emitted.stdout}{emitted.stderr}")
                return 1

            gas_c_abi_object = working / "integer128-gas32-c-abi.o"
            assembled = subprocess.run(
                [os.environ.get("AS") or "as", "--32", "-o",
                 str(gas_c_abi_object), str(gas_c_abi_assembly)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if assembled.returncode:
                print("gas32/C int128 ABI assembly failed\n"
                      f"{assembled.stdout}{assembled.stderr}")
                return 1

            gas_c_abi_runtime = working / "integer128-gas32-c-abi"
            c_build = subprocess.run(
                [gcc, "-m32", "-no-pie", "-std=c99", "-Wall", "-Wextra", "-Werror",
                 str(gas_c_abi_object),
                 str(root / "tests/compiler-support/integer128-gas32-c-abi-main.c"),
                 str(root / "tests/compiler-support/integer128-gas32-c-abi.c"),
                 str(root / "src/rtlib/math/int128.c"), "-o", str(gas_c_abi_runtime)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if c_build.returncode:
                print("gas32/C int128 ABI link failed\n"
                      f"{c_build.stdout}{c_build.stderr}")
                return 1
            executed = subprocess.run([str(gas_c_abi_runtime)], cwd=working,
                                      text=True, capture_output=True, timeout=30, check=False)
            if executed.returncode:
                print("gas32/C int128 ABI runtime failed\n"
                      f"{executed.stdout}{executed.stderr}")
                return 1
            print("gas32/C int128 aggregate ABI passed")

            c_i386_abi_source = root / "tests/compiler-support/integer128-gas32-c-abi.bas"
            c_i386_abi_output = working / "integer128-gcc-i386-c-abi.c"
            c_i386_abi_emit = subprocess.run(
                [*compiler_args, "-i",
                 str(root / "inc"), "-target", "linux", "-arch", "686",
                 "-gen", "gcc", "-r", str(c_i386_abi_source), "-o",
                 str(c_i386_abi_output)], cwd=working, text=True,
                capture_output=True, timeout=120, check=False)
            if c_i386_abi_emit.returncode:
                print("gcc/i386 C int128 ABI emission failed\n"
                      f"{c_i386_abi_emit.stdout}{c_i386_abi_emit.stderr}")
                return 1
            c_i386_abi_runtime = working / "integer128-gcc-i386-c-abi"
            c_build = subprocess.run(
                [gcc, "-m32", "-std=c99", "-Wall", "-Wextra",
                 "-Werror=implicit-function-declaration", str(c_i386_abi_output),
                 str(root / "tests/compiler-support/integer128-gas32-c-abi-main.c"),
                 str(root / "tests/compiler-support/integer128-gas32-c-abi.c"),
                 str(root / "src/rtlib/math/int128.c"), "-o", str(c_i386_abi_runtime)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if c_build.returncode:
                print("gcc/i386 C int128 ABI link failed\n"
                      f"{c_build.stdout}{c_build.stderr}")
                return 1
            executed = subprocess.run([str(c_i386_abi_runtime)], cwd=working,
                                      text=True, capture_output=True, timeout=30, check=False)
            if executed.returncode:
                print("gcc/i386 C int128 ABI runtime failed\n"
                      f"{executed.stdout}{executed.stderr}")
                return 1
            print("gcc/i386 C int128 aggregate ABI passed")

            if clang is not None:
                # Win32 cannot run in this Linux test process, but Clang can
                # assemble both 32-bit outputs into COFF without a Windows SDK.
                gas_win32_assembly = working / "integer128-gas32-win32.asm"
                emitted = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "win32", "-gen", "gas",
                     "-r", str(root / "tests/compiler-support/integer128-gas32.bas"),
                     "-o", str(gas_win32_assembly)], cwd=working, text=True,
                    capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print("gas32/win32 int128 emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1

                gas_win32_object = working / "integer128-gas32-win32.obj"
                assembled = subprocess.run(
                    [clang, "-target", "i686-w64-windows-gnu", "-c", "-x",
                     "assembler", str(gas_win32_assembly), "-o", str(gas_win32_object)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if (assembled.returncode or not gas_win32_object.is_file() or
                        int.from_bytes(gas_win32_object.read_bytes()[:2], "little") != 0x14C):
                    print("gas32/win32 int128 COFF assembly failed\n"
                          f"{assembled.stdout}{assembled.stderr}")
                    return 1
                print("gas32/win32 int128 ABI lowering passed")

                gcc_win32_source = working / "integer128-gcc-i386-win32.c"
                emitted = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "win32", "-arch", "686",
                     "-gen", "gcc", "-r",
                     str(root / "tests/compiler-support/integer128-c-i386.bas"),
                     "-o", str(gcc_win32_source)], cwd=working, text=True,
                    capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print("gcc/i386-win32 int128 emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1

                gcc_win32_object = working / "integer128-gcc-i386-win32.obj"
                compiled = subprocess.run(
                    [clang, "--target=i686-w64-windows-gnu", "-c",
                     "-Wno-implicit-function-declaration", str(gcc_win32_source),
                     "-o", str(gcc_win32_object)], cwd=working, text=True,
                    capture_output=True, timeout=60, check=False)
                if (compiled.returncode or not gcc_win32_object.is_file() or
                        int.from_bytes(gcc_win32_object.read_bytes()[:2], "little") != 0x14C):
                    print("gcc/i386-win32 int128 COFF compilation failed\n"
                          f"{compiled.stdout}{compiled.stderr}")
                    return 1
                print("gcc/i386-win32 int128 ABI lowering passed")

            if llc is not None:
                llvm_source = root / "tests/compiler-support/integer128-gas32.bas"
                llvm_ir = working / "integer128-llvm-i386.ll"
                llvm_emit = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "linux-x86", "-gen", "llvm",
                     "-r", str(llvm_source), "-o", str(llvm_ir)],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if llvm_emit.returncode:
                    print("llvm/i386 int128 IR emission failed\n"
                          f"{llvm_emit.stdout}{llvm_emit.stderr}")
                    return 1
                llvm_object = working / "integer128-llvm-i386.o"
                lowered = subprocess.run(
                    [llc, "-mtriple=i386-pc-linux-gnu", "-filetype=obj",
                     str(llvm_ir), "-o", str(llvm_object)], cwd=working,
                    text=True, capture_output=True, timeout=60, check=False)
                if lowered.returncode:
                    print("llvm/i386 int128 lowering failed\n"
                          f"{lowered.stdout}{lowered.stderr}")
                    return 1
                llvm_runtime = working / "integer128-llvm-i386"
                c_build = subprocess.run(
                    [gcc, "-m32", "-no-pie", "-std=c99", "-Wall", "-Wextra", "-Werror",
                     str(llvm_object),
                     str(root / "tests/compiler-support/integer128-gas32-main.c"),
                     str(root / "src/rtlib/math/int128.c"), "-o", str(llvm_runtime)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print("llvm/i386 int128 link failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                executed = subprocess.run([str(llvm_runtime)], cwd=working,
                                          text=True, capture_output=True, timeout=30, check=False)
                if executed.returncode:
                    print("llvm/i386 int128 runtime failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print("llvm/i386 int128 lowering passed")

                llvm_c_abi_source = root / "tests/compiler-support/integer128-gas32-c-abi.bas"
                llvm_c_abi_ir = working / "integer128-llvm-i386-c-abi.ll"
                emitted = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "linux-x86", "-gen", "llvm",
                     "-r", str(llvm_c_abi_source), "-o", str(llvm_c_abi_ir)],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print("llvm/i386 C int128 ABI emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1

                llvm_c_abi_object = working / "integer128-llvm-i386-c-abi.o"
                lowered = subprocess.run(
                    [llc, "-mtriple=i386-pc-linux-gnu", "-filetype=obj",
                     str(llvm_c_abi_ir), "-o", str(llvm_c_abi_object)], cwd=working,
                    text=True, capture_output=True, timeout=60, check=False)
                if lowered.returncode:
                    print("llvm/i386 C int128 ABI lowering failed\n"
                          f"{lowered.stdout}{lowered.stderr}")
                    return 1

                llvm_c_abi_runtime = working / "integer128-llvm-i386-c-abi"
                c_build = subprocess.run(
                    [gcc, "-m32", "-no-pie", "-std=c99", "-Wall", "-Wextra", "-Werror",
                     str(llvm_c_abi_object),
                     str(root / "tests/compiler-support/integer128-gas32-c-abi-main.c"),
                     str(root / "tests/compiler-support/integer128-gas32-c-abi.c"),
                     str(root / "src/rtlib/math/int128.c"), "-o", str(llvm_c_abi_runtime)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print("llvm/i386 C int128 ABI link failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                executed = subprocess.run([str(llvm_c_abi_runtime)], cwd=working,
                                          text=True, capture_output=True, timeout=30, check=False)
                if executed.returncode:
                    print("llvm/i386 C int128 ABI runtime failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print("llvm/i386 C int128 aggregate ABI passed")

                llvm_win32_ir = working / "integer128-llvm-win32-i386.ll"
                emitted = subprocess.run(
                    [*compiler_args, "-i",
                     str(root / "inc"), "-target", "win32", "-arch", "686",
                     "-gen", "llvm", "-r", str(llvm_source), "-o", str(llvm_win32_ir)],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print("llvm/win32-i386 int128 ABI emission failed\n"
                          f"{emitted.stdout}{emitted.stderr}")
                    return 1
                llvm_win32_object = working / "integer128-llvm-win32-i386.o"
                lowered = subprocess.run(
                    [llc, "-mtriple=i686-w64-windows-gnu", "-filetype=obj",
                     str(llvm_win32_ir), "-o", str(llvm_win32_object)], cwd=working,
                    text=True, capture_output=True, timeout=60, check=False)
                if (lowered.returncode or not llvm_win32_object.is_file() or
                        int.from_bytes(llvm_win32_object.read_bytes()[:2], "little") != 0x14C):
                    print("llvm/win32-i386 int128 ABI lowering failed\n"
                          f"{lowered.stdout}{lowered.stderr}")
                    return 1
                print("llvm/win32-i386 int128 ABI lowering passed")

            windows_source = root / "tests/compiler-support/integer128-gas64-windows.bas"
            windows_assembly = working / "integer128-gas64-windows.asm"
            windows_emit = subprocess.run(
                [*compiler_args, "-i",
                 str(root / "inc"), "-target", "win64", "-gen", "gas64",
                 "-r", str(windows_source), "-o", str(windows_assembly)],
                cwd=working, text=True, capture_output=True, timeout=120, check=False)
            if windows_emit.returncode:
                print("gas64/win64 int128 emission failed\n"
                      f"{windows_emit.stdout}{windows_emit.stderr}")
                return 1
            if clang is not None:
                windows_object = working / "integer128-gas64-windows.o"
                assembled = subprocess.run(
                    [clang, "-target", "x86_64-w64-windows-gnu", "-c",
                     "-x", "assembler", str(windows_assembly), "-o",
                     str(windows_object)], cwd=working, text=True,
                    capture_output=True, timeout=60, check=False)
                if assembled.returncode:
                    print("gas64/win64 int128 assembly failed\n"
                          f"{assembled.stdout}{assembled.stderr}")
                    return 1
                print("gas64/win64 int128 assembly passed")
            else:
                print("gas64/win64 int128 call lowering passed")

        # GAS64 emits ELF/COFF sections. Native Darwin's Mach-O fixtures use
        # the C and LLVM backends above; cross-target emission remains tested.
        if architecture == "x86_64" and system != "darwin":
            source = root / "tests/compiler-support/integer128-layout.bas"
            executable = working / "gas64-int128-layout"
            command = [*compiler_args,
                       "-i", str(root / "inc"), "-gen", "gas64",
                       str(source), "-x", str(executable)]
            try:
                built = subprocess.run(command, cwd=working, text=True,
                                       capture_output=True, timeout=120, check=False)
                if built.returncode:
                    print(f"gas64/int128-layout: compilation failed\n{built.stdout}{built.stderr}")
                    return 1
                executed = subprocess.run([str(executable)], cwd=working, text=True,
                                          capture_output=True, timeout=30, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas64/int128-layout: {error}")
                return 1
            if executed.returncode or executed.stdout.strip() != "128-bit integer layout passed":
                print(f"gas64/int128-layout: execution failed\n{executed.stdout}{executed.stderr}")
                return 1
            print("gas64/int128-layout: passed")

            if incomplete_extern_object is not None:
                source = root / "tests/compiler-support/incomplete-extern.bas"
                executable = working / "gas64-incomplete-extern"
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-gen", "gas64",
                           str(source), str(incomplete_extern_object),
                           "-x", str(executable)]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print("gas64/incomplete-extern: compilation failed\n"
                              f"{built.stdout}{built.stderr}")
                        return 1
                    executed = subprocess.run([str(executable)], cwd=working,
                                              text=True, capture_output=True,
                                              timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas64/incomplete-extern: {error}")
                    return 1
                if executed.returncode or executed.stdout.strip():
                    print("gas64/incomplete-extern: execution failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print("gas64/incomplete-extern: passed")

            source = root / "tests/compiler-support/integer128-layout-gas32.bas"
            assembly = working / "gas-int128-layout.asm"
            command = [*compiler_args,
                       "-i", str(root / "inc"), "-target", "linux-x86",
                       "-gen", "gas", "-r", str(source), "-o", str(assembly)]
            try:
                built = subprocess.run(command, cwd=working, text=True,
                                       capture_output=True, timeout=120, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas/int128-layout: {error}")
                return 1
            if built.returncode:
                print(f"gas/int128-layout: compilation failed\n{built.stdout}{built.stderr}")
                return 1
            emitted = assembly.read_text(encoding="utf-8")
            if (".balign 16" not in emitted or
                    ".lcomm\tINTEGER128VALUE$,16" not in emitted or
                    ".lcomm\tRECORDVALUE$,48" not in emitted):
                print(f"gas/int128-layout: storage alignment mismatch\n{emitted}")
                return 1
            print("gas/int128-layout: passed")

            source = root / "tests/compiler-support/integer128-arithmetic.bas"
            executable = working / "gas64-int128-arithmetic"
            command = [*compiler_args,
                       "-i", str(root / "inc"), "-gen", "gas64",
                       str(source), "-x", str(executable)]
            try:
                built = subprocess.run(command, cwd=working, text=True,
                                       capture_output=True, timeout=120, check=False)
                if built.returncode:
                    print(f"gas64/int128-arithmetic: compilation failed\n{built.stdout}{built.stderr}")
                    return 1
                executed = subprocess.run([str(executable)], cwd=working, text=True,
                                          capture_output=True, timeout=30, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas64/int128-arithmetic: {error}")
                return 1
            if executed.returncode or executed.stdout.strip() != "128-bit integer arithmetic passed":
                print(f"gas64/int128-arithmetic: execution failed\n{executed.stdout}{executed.stderr}")
                return 1
            print("gas64/int128-arithmetic: passed")

            if int128_input_object is not None:
                source = root / "tests/compiler-support/integer128-abi-input.bas"
                executable = working / "gas64-int128-abi-input"
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-gen", "gas64",
                           str(source), str(int128_input_object), "-x", str(executable)]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print(f"gas64/int128-abi-input: compilation failed\n{built.stdout}{built.stderr}")
                        return 1
                    executed = subprocess.run([str(executable)], cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas64/int128-abi-input: {error}")
                    return 1
                if executed.returncode or executed.stdout.strip() != "128-bit argument ABI passed":
                    print(f"gas64/int128-abi-input: execution failed\n{executed.stdout}{executed.stderr}")
                    return 1
                print("gas64/int128-abi-input: passed")

            if int128_object is not None:
                source = root / "tests/compiler-support/integer128-abi.bas"
                executable = working / "gas64-int128-abi"
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-gen", "gas64",
                           str(source), str(int128_object), "-x", str(executable)]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print(f"gas64/int128-abi: compilation failed\n{built.stdout}{built.stderr}")
                        return 1
                    executed = subprocess.run([str(executable)], cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas64/int128-abi: {error}")
                    return 1
                if executed.returncode or executed.stdout.strip() != "128-bit C ABI passed":
                    print(f"gas64/int128-abi: execution failed\n{executed.stdout}{executed.stderr}")
                    return 1
                print("gas64/int128-abi: passed")

            source = root / "tests/compiler-support/integer128-arithmetic.bas"
            assembly = working / "gas-int128-arithmetic.asm"
            command = [*compiler_args,
                       "-i", str(root / "inc"), "-target", "linux-x86",
                       "-gen", "gas", "-r", str(source), "-o", str(assembly)]
            try:
                built = subprocess.run(command, cwd=working, text=True,
                                       capture_output=True, timeout=120, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas/int128-arithmetic: {error}")
                return 1
            if built.returncode:
                print(f"gas/int128-arithmetic: compilation failed\n{built.stdout}{built.stderr}")
                return 1
            assembled = subprocess.run(
                [os.environ.get("AS") or "as", "--32", "-o",
                 str(working / "gas-int128-arithmetic.o"), str(assembly)],
                cwd=working, text=True, capture_output=True, timeout=60, check=False)
            if assembled.returncode:
                print(f"gas/int128-arithmetic: assembly failed\n{assembled.stdout}{assembled.stderr}")
                return 1
            print("gas/int128-arithmetic: emitted")

            thread_local_source = root / "tests/compiler-support/thread-local.bas"
            executable = working / "gas64-thread-local"
            command = [*compiler_args,
                       "-i", str(root / "inc"), "-gen", "gas64", "-mt",
                       str(thread_local_source), "-x", str(executable)]
            try:
                built = subprocess.run(command, cwd=working, text=True,
                                       capture_output=True, timeout=120, check=False)
                if built.returncode:
                    print(f"gas64/thread-local: compilation failed\n{built.stdout}{built.stderr}")
                    return 1
                executed = subprocess.run([str(executable)], cwd=working, text=True,
                                          capture_output=True, timeout=30, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas64/thread-local: {error}")
                return 1
            if executed.returncode or executed.stdout.strip() != "thread-local storage passed":
                print(f"gas64/thread-local: execution failed\n{executed.stdout}{executed.stderr}")
                return 1
            print("gas64/thread-local: passed")

            if thread_local_extern_object is not None:
                source = root / "tests/compiler-support/thread-local-extern.bas"
                executable = working / "gas64-thread-local-extern"
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-gen", "gas64", "-mt",
                           str(source), str(thread_local_extern_object),
                           "-x", str(executable)]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print("gas64/thread-local-extern: compilation failed\n"
                              f"{built.stdout}{built.stderr}")
                        return 1
                    executed = subprocess.run([str(executable)], cwd=working,
                                              text=True, capture_output=True,
                                              timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas64/thread-local-extern: {error}")
                    return 1
                if (executed.returncode or
                        executed.stdout.strip() != "thread-local external passed"):
                    print("gas64/thread-local-extern: execution failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print("gas64/thread-local-extern: passed")

                command = [*compiler_args,
                           "-i", str(root / "inc"), "-dylib", "-gen", "gas64",
                           "-mt", str(source), str(thread_local_extern_object),
                           "-x", str(working / "gas64-thread-local-extern-dll")]
                try:
                    rejected = subprocess.run(command, cwd=working, text=True,
                                              capture_output=True, timeout=120, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas64/thread-local-extern-dll: {error}")
                    return 1
                diagnostic = rejected.stdout + rejected.stderr
                if (rejected.returncode == 0 or
                        "Thread Local declaration is not supported for this type, initializer, or target"
                        not in diagnostic or "Segmentation violation" in diagnostic):
                    print("gas64/thread-local-extern-dll: invalid rejection\n"
                          f"{diagnostic}")
                    return 1
                print("gas64/thread-local-extern-dll: safely rejected")

            source = root / "tests/compiler-support/thread-local-managed.bas"
            command = [*compiler_args,
                       "-i", str(root / "inc"), "-gen", "gas64", "-mt", "-c",
                       str(source), "-o", str(working / "gas64-thread-local-managed.o")]
            try:
                rejected = subprocess.run(command, cwd=working, text=True,
                                          capture_output=True, timeout=120, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas64/thread-local-managed: {error}")
                return 1
            diagnostic = rejected.stdout + rejected.stderr
            if (rejected.returncode == 0 or
                    "Thread Local declaration is not supported for this type, initializer, or target" not in diagnostic or
                    "Segmentation violation" in diagnostic):
                print(f"gas64/thread-local-managed: invalid rejection\n{diagnostic}")
                return 1
            print("gas64/thread-local-managed: safely rejected")

            command = [*compiler_args,
                       "-i", str(root / "inc"), "-target", "linux-x86",
                       "-gen", "gas", "-mt", "-r", str(thread_local_source),
                       "-o", str(working / "gas-thread-local.asm")]
            try:
                built = subprocess.run(command, cwd=working, text=True,
                                       capture_output=True, timeout=120, check=False)
            except (OSError, subprocess.TimeoutExpired) as error:
                print(f"gas/thread-local: {error}")
                return 1
            if built.returncode:
                print(f"gas/thread-local: compilation failed\n{built.stdout}{built.stderr}")
                return 1
            print("gas/thread-local: emitted")

            if thread_local_extern_object is not None:
                thread_local_extern_source = root / "tests/compiler-support/thread-local-extern.bas"
                assembly = working / "gas-thread-local-extern.asm"
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-target", "linux-x86",
                           "-gen", "gas", "-mt", "-r",
                           str(thread_local_extern_source),
                           "-o", str(assembly)]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas/thread-local-extern: {error}")
                    return 1
                if built.returncode:
                    print(f"gas/thread-local-extern: assembly emission failed\n{built.stdout}{built.stderr}")
                    return 1
                emitted = assembly.read_text(encoding="utf-8").lower()
                if "@ntpoff" not in emitted or "gs:" not in emitted:
                    print(f"gas/thread-local-extern: missing ELF TLS access\n{emitted}")
                    return 1
                assembled = subprocess.run(
                    [os.environ.get("AS") or "as", "--32", "-o",
                     str(working / "gas-thread-local-extern.o"), str(assembly)],
                    cwd=working, text=True, capture_output=True, timeout=60, check=False)
                if assembled.returncode:
                    print("gas/thread-local-extern: assembly failed\n"
                          f"{assembled.stdout}{assembled.stderr}")
                    return 1
                print("gas/thread-local-extern: passed")

        if "clang" in backends:
            if clang is None:
                print("clang: configured compiler is unavailable")
                return 1
            # LP64 does not determine uint64_t's C spelling. Check generated
            # prototypes without needing another target's SDK or linker.
            source = working / "builtin-target.bas"
            source.write_text('#include once "builtin.bi"\n'
                              'dim value as ulongint = &h0123456789abcdefull\n'
                              'value = __builtin_bswap64(value)\n'
                              'dim shared copyBytes as function cdecl( byval as any ptr, '
                              'byval as const any ptr, byval as __fb_builtin_size_t ) '
                              'as any ptr = @__builtin_memcpy\n', encoding="utf-8")
            targets = (
                ("clang", "linux", "x86_64", "x86_64-linux-gnu"),
                ("clang", "darwin", "x86_64", "x86_64-apple-darwin"),
                ("clang", "darwin", "aarch64", "aarch64-apple-darwin"),
                ("clang", "freebsd", "x86_64", "x86_64-unknown-freebsd"),
                ("clang", "openbsd", "x86_64", "x86_64-unknown-openbsd"),
                ("clang", "netbsd", "x86_64", "x86_64-unknown-netbsd"),
                # Darwin also runs Clang behind the system gcc command.
                ("gcc", "darwin", "x86_64", "x86_64-apple-darwin"),
                ("gcc", "darwin", "aarch64", "aarch64-apple-darwin"),
                ("clang", "win32", "aarch64", "aarch64-w64-mingw32"),
                ("gcc", "win32", "aarch64", "aarch64-w64-mingw32"),
            )
            for backend, target, arch, triple in targets:
                label = f"{backend}/{target}-{arch}"
                command = [*compiler_args,
                           "-i", str(root / "inc"), "-gen", backend, "-r",
                           "-target", target, "-arch", arch, str(source)]
                try:
                    emitted = subprocess.run(command, cwd=working, text=True,
                                             capture_output=True, timeout=120, check=False)
                    if emitted.returncode:
                        print(f"{label}: C emission failed\n{emitted.stdout}{emitted.stderr}")
                        return 1
                    checked = subprocess.run([clang, "--target=" + triple, "-fsyntax-only",
                                              "-nostdinc", str(source.with_suffix(".c"))],
                                             cwd=working, text=True, capture_output=True,
                                             timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"{label}: {error}")
                    return 1
                if checked.returncode:
                    print(f"{label}: C declarations failed\n{checked.stdout}{checked.stderr}")
                    return 1
                print(f"{label}: C declarations passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-backends.py
