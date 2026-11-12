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
import sys
import tempfile


def check_i386_clongdouble_abi(root: Path, fbc: Path, working: Path,
                               clang: str | None, llc: str | None) -> int:
    """Run CLongDouble result and argument checks through i386 C ABIs."""
    basic_source = working / "clongdouble-i386.bas"
    basic_source.write_text(
        '#include once "crt/longdouble.bi"\n'
        'Declare Function c2fbLongDoubleSum CDecl Alias "c2fbLongDoubleSum" ( _\n'
        '    ByVal lhs As CLongDouble, ByVal rhs As CLongDouble) As CLongDouble\n'
        'Function c2fbLongDoubleCheck CDecl Alias "c2fbLongDoubleCheck" ( _\n'
        '    ByVal lhs As CLongDouble, ByVal rhs As CLongDouble) As Integer\n'
        '    Dim As CLongDouble result = c2fbLongDoubleSum(lhs, rhs)\n'
        '    Return CInt(result = CLongDoubleFromDoubleValue(3.75#))\n'
        'End Function\n'
        'Function c2fbLongDoubleAdd CDecl Alias "c2fbLongDoubleAdd" ( _\n'
        '    ByVal lhs As CLongDouble, ByVal rhs As CLongDouble) As CLongDouble\n'
        '    Return lhs + rhs\n'
        'End Function\n'
        'Function c2fbLongDoubleForward CDecl Alias "c2fbLongDoubleForward" ( _\n'
        '    ByVal value As CLongDouble) As CLongDouble\n'
        '    Return c2fbLongDoubleSum( _\n'
        '        value, CLongDoubleFromDoubleValue(0.5#))\n'
        'End Function\n',
        encoding="utf-8",
    )
    c_source = working / "clongdouble-i386.c"
    c_source.write_text(
        "long double c2fbLongDoubleSum(long double lhs, long double rhs) "
        "{ return lhs + rhs; }\n"
        "int c2fbLongDoubleCheck(long double lhs, long double rhs);\n"
        "long double c2fbLongDoubleAdd(long double lhs, long double rhs);\n"
        "long double c2fbLongDoubleForward(long double value);\n"
        "int main(void) {\n"
        "    if (c2fbLongDoubleCheck(1.25L, 2.5L) != -1) return 1;\n"
        "    if (c2fbLongDoubleAdd(1.25L, 2.5L) != 3.75L) return 2;\n"
        "    if (c2fbLongDoubleForward(1.25L) != 1.75L) return 3;\n"
        "    return 0;\n"
        "}\n",
        encoding="utf-8",
    )

    gcc = os.environ.get("GCC") or "gcc"
    helper_object = working / "longdouble_ops-i386.o"
    c_object = working / "clongdouble-i386.o"
    c_build = subprocess.run(
        [gcc, "-m32", "-std=c99", "-Wall", "-Wextra", "-Werror",
         "-fno-strict-aliasing", "-c", str(root / "src/rtlib/math/longdouble_ops.c"),
         "-o", str(helper_object)],
        cwd=working, text=True, capture_output=True, timeout=60, check=False,
    )
    if c_build.returncode:
        print("GAS32 CLongDouble runtime helper compilation failed\n"
              f"{c_build.stdout}{c_build.stderr}")
        return 1
    c_build = subprocess.run(
        [gcc, "-m32", "-std=c99", "-Wall", "-Wextra", "-Werror", "-c",
         str(c_source), "-o", str(c_object)],
        cwd=working, text=True, capture_output=True, timeout=60, check=False,
    )
    if c_build.returncode:
        print("GAS32 CLongDouble C driver compilation failed\n"
              f"{c_build.stdout}{c_build.stderr}")
        return 1

    backends = [("gas", "gas"), ("gcc", "gcc")]
    if llc is not None:
        backends.append(("llvm", "llvm"))
    for label, generator in backends:
        object_file = working / f"clongdouble-i386-{label}.o"
        environment = os.environ.copy()
        assembler = shutil.which(os.environ.get("AS") or "as")
        if assembler is not None:
            environment["AS"] = assembler
        environment["GCC"] = gcc
        if clang is not None:
            environment["CLANG"] = clang
        if llc is not None:
            environment["LLC"] = llc
        emitted = subprocess.run(
            [str(fbc.resolve()), "-prefix", str(root), "-i", str(root / "inc"),
             "-target", "linux-x86", "-gen", generator, "-c",
             str(basic_source), "-o", str(object_file)],
            cwd=working, env=environment, text=True, capture_output=True,
            timeout=120, check=False,
        )
        if emitted.returncode:
            print(f"{label}/i386 CLongDouble emission failed\n"
                  f"{emitted.stdout}{emitted.stderr}")
            return 1

        executable = working / f"clongdouble-i386-{label}"
        linked = subprocess.run(
            [gcc, "-m32", "-no-pie", str(c_object), str(object_file),
             str(helper_object), "-lm", "-o", str(executable)],
            cwd=working, text=True, capture_output=True, timeout=60, check=False,
        )
        if linked.returncode:
            print(f"{label}/i386 CLongDouble link failed\n"
                  f"{linked.stdout}{linked.stderr}")
            return 1
        executed = subprocess.run(
            [str(executable)], cwd=working, text=True, capture_output=True,
            timeout=30, check=False,
        )
        if executed.returncode:
            print(f"{label}/i386 CLongDouble runtime failed\n"
                  f"{executed.stdout}{executed.stderr}")
            return 1
        print(f"{label}/i386 CLongDouble result ABI passed")
    return 0


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
        ("fbcunit-report", "tests/compiler-support/fbcunit-report-contract.bas", ""),
        ("runtime-aliases", "tests/compiler-support/c-runtime-aliases.bas", ""),
        ("readonly-entries", "tests/compiler-support/c-readonly-entry-reachability.bas", ""),
        ("unused-parameters", "tests/compiler-support/c-unused-parameters-after-stores.bas",
         "C_UNUSED_PARAMETERS_AFTER_STORES_PASS"),
        ("opaque-local-metadata", "tests/compiler-support/c-opaque-local-metadata.bas",
         "C_OPAQUE_LOCAL_METADATA_PASS"),
        ("error-capture-layout", "tests/compiler-support/c-error-capture-layout.bas",
         "C_ERROR_CAPTURE_LAYOUT_PASS"),
        ("startup-declarations", "tests/compiler-support/c-startup-declarations.bas",
         "C_STARTUP_DECLARATIONS_PASS\nC_STARTUP_DECLARATIONS_CLEANUP_PASS"),
        ("storage-alignment-owner", "tests/compiler-support/c-storage-alignment-owner.bas", ""),
        ("qualified-float-literals", "tests/compiler-support/c-qualified-float-literals.bas",
         "C_QUALIFIED_FLOAT_LITERALS_PASS"),
        ("indirect-integer-ranges", "tests/compiler-support/c-indirect-integer-ranges.bas",
         "C_INDIRECT_INTEGER_RANGES_PASS"),
        ("indirect-ranges-managed-iif", "tests/compiler-support/c-indirect-ranges-managed-iif.bas",
         "C_INDIRECT_RANGES_MANAGED_IIF_PASS"),
        ("gosub-context-dependency", "tests/compiler-support/c-gosub-context-dependency.bas",
         "GOSUB_STORAGE_DEPENDENCY_PASS"),
        ("gosub-cleanup-dependency", "tests/compiler-support/c-gosub-cleanup-dependency.bas",
         "GOSUB_ERROR_CLEANUP_DEPENDENCY_PASS"),
        ("gosub-handler-labels", "tests/compiler-support/c-gosub-handler-labels.bas",
         "GOSUB_HANDLER_LABELS_PASS"),
        ("reserved-names", "tests/compiler-support/c-reserved-names.bas",
         "C_RESERVED_NAMES_PASS\nC_RESERVED_DESTRUCTOR_PASS"),
        ("terminating-context", "tests/compiler-support/c-terminating-context.bas",
         "C_TERMINATING_CONTEXT_PASS"),
        ("unused-pointer-stores", "tests/compiler-support/c-unused-pointer-stores.bas",
         "POINTER_STORE_VALUES_PASS"),
        ("array-descriptor-end", "tests/compiler-support/c-array-descriptor-end.bas",
         "ARRAY_DESCRIPTOR_END_PASS"),
        ("custom-entry", "tests/compiler-support/c-custom-entry.bas",
         "CUSTOM_ENTRY_VALUES_PASS"),
        ("named-memory-asm", "tests/compiler-support/c-named-memory-asm.bas",
         "C_NAMED_MEMORY_ASM_PASS"),
        ("static-erased-callbacks", "tests/compiler-support/c-static-erased-callbacks.bas",
         "C_STATIC_ERASED_CALLBACKS_PASS"),
        ("predefined-timestamps", "tests/compiler-support/c-predefined-timestamps.bas",
         "PREDEFINED_TIMESTAMPS_PASS"),
        ("virtual-receiver", "tests/compiler-support/c-virtual-receiver-check.bas",
         "VIRTUAL_RECEIVER_CHECK_PASS"),
        ("virtual-receiver-checked", "tests/compiler-support/c-virtual-receiver-check.bas",
         "VIRTUAL_RECEIVER_CHECK_PASS"),
        ("pointer-values", "tests/compiler-support/c-pointer-values.bas", "POINTER_VALUES_PASS 330"),
        ("pointer-order", "tests/compiler-support/c-pointer-order.bas", "POINTER_ORDER_VALUES_PASS"),
        ("pointer-null-order", "tests/compiler-support/c-pointer-null-order.bas", "POINTER_NULL_ORDER_PASS"),
        ("array-values", "tests/compiler-support/c-array-values.bas", "ARRAY_VALUE_MATRIX_PASS"),
        ("dispatch-values", "tests/compiler-support/c-dispatch-values.bas", "DISPATCH_VALUES_PASS"),
        ("gosub-values", "tests/compiler-support/c-gosub-values.bas", "GOSUB_VALUES_PASS"),
        ("label-flag-layout", "tests/compiler-support/c-label-flag-layout.bas", "C_LABEL_FLAG_LAYOUT_PASS"),
        ("label-flag-gosub", "tests/compiler-support/label-flag-gosub.bas", "LABEL_FLAG_GOSUB_PASS"),
        ("member-pointer-slots", "tests/compiler-support/c-member-pointer-slots.bas", "MEMBER_POINTER_SLOTS_PASS"),
        ("record-layout", "tests/compiler-support/c-record-layout.bas", "RECORD_LAYOUT_PASS 6"),
        ("packed-descriptors", "tests/compiler-support/c-packed-descriptors.bas", "PACKED_DESCRIPTORS_PASS"),
        ("packed-descriptor-tail", "tests/compiler-support/c-packed-descriptor-tail.bas", "PACKED_DESCRIPTOR_TAIL_PASS"),
        ("packed-descriptor-init", "tests/compiler-support/c-packed-descriptor-init.bas", "PACKED_DESCRIPTOR_INIT_PASS"),
        ("packed-anonymous-descriptors", "tests/compiler-support/c-packed-anonymous-descriptors.bas", "PACKED_ANONYMOUS_DESCRIPTORS_PASS"),
        ("external-prototypes", "tests/compiler-support/c-external-prototypes.bas", "EXTERNAL_PROTOTYPES_PASS"),
    ]
    with tempfile.TemporaryDirectory(prefix="fbc-backends-") as temporary:
        working = Path(temporary)
        native_support = subprocess.run(
            [sys.executable, '-B', str(root / 'tests/compiler-support/test_native_c_support.py')],
            cwd=working, text=True, capture_output=True, timeout=30, check=False)
        if native_support.returncode:
            print('native compiler support contracts failed\n' + native_support.stdout + native_support.stderr)
            return 1
        # Source diagnostics must survive changes in C warning cleanup and
        # remain consistent for the native assembly producers as well.
        source_warnings = subprocess.run(
            [sys.executable, "-B", str(root / "build_scripts/test-source-warnings.py"),
             "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
             "--work-dir", str(working)],
            cwd=root, text=True, capture_output=True, check=False)
        if source_warnings.returncode:
            print("source warning regression failed\n"
                  f"{source_warnings.stdout}{source_warnings.stderr}")
            return 1
        print(source_warnings.stdout.strip())
        # Test the C parser directly so stale installed runtime archives cannot
        # conceal a missing CRT API or a loss of native long-double precision.
        locale_test = working / ("longdouble-locale.exe" if os.name == "nt"
                                 else "longdouble-locale")
        locale_build = subprocess.run(
            [os.environ.get("GCC") or "gcc", "-std=c99", "-Wall", "-Wextra",
             "-Werror", str(root / "tests/compiler-support/longdouble-string-locale.c"),
             str(root / "src/rtlib/math/longdouble_ops.c"), "-lm",
             "-o", str(locale_test)],
            cwd=working, text=True, capture_output=True, timeout=60, check=False)
        if locale_build.returncode:
            print("long-double locale test compilation failed\n"
                  f"{locale_build.stdout}{locale_build.stderr}")
            return 1
        locale_run = subprocess.run(
            [str(locale_test)], cwd=working, text=True, capture_output=True,
            timeout=30, check=False)
        if locale_run.returncode:
            print("long-double locale test failed\n"
                  f"{locale_run.stdout}{locale_run.stderr}")
            return 1
        print(locale_run.stdout.strip())
        # The dispatcher restores the exact signature before invoking an erased
        # callback. Check both C producers and dialects independently of GAS64.
        callback_command = [sys.executable,
                            str(root / "tests/compiler-support/c-generic-callback-values.py"),
                            "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
                            "--gcc", os.environ.get("GCC") or "gcc", "--work-dir", str(working),
                            "--standard", "gnu23", "--optimization", "2", "--mode", "e"]
        if clang:
            callback_command += ["--clang", clang]
        callback_check = subprocess.run(callback_command, cwd=working, text=True,
                                        capture_output=True, timeout=180, check=False)
        if callback_check.returncode:
            print(callback_check.stdout + callback_check.stderr)
            return 1
        print(callback_check.stdout.strip())

        names_check = subprocess.run(
            [sys.executable, str(root / "tests/compiler-support/c-longdouble-temporary-names.py"),
             "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
             "--root", str(root), "--work-dir", str(working)],
            cwd=working, text=True, capture_output=True, timeout=600, check=False)
        if names_check.returncode:
            print(names_check.stdout + names_check.stderr)
            return 1
        print(names_check.stdout.strip())
        # Native C declarations can be incompatible under LTO even when their
        # byte layouts agree. Keep this check alongside the value fixtures.
        header_command = [sys.executable,
                          str(root / "tests/compiler-support/c-anonymous-header.py"),
                          "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
                          "--gcc", os.environ.get("GCC") or "gcc", "--work-dir", str(working)]
        header_check = subprocess.run(header_command, cwd=working, text=True,
                                      capture_output=True, timeout=180, check=False)
        if header_check.returncode:
            print(header_check.stdout + header_check.stderr)
            return 1
        print(header_check.stdout.strip())
        pointer_command = [sys.executable,
                           str(root / "tests/compiler-support/c-record-pointer-dependencies.py"),
                           "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
                           "--gcc", os.environ.get("GCC") or "gcc", "--work-dir", str(working)]
        if clang:
            pointer_command += ["--clang", clang]
        pointer_check = subprocess.run(pointer_command, cwd=working, text=True,
                                       capture_output=True, timeout=180, check=False)
        if pointer_check.returncode:
            print(pointer_check.stdout + pointer_check.stderr)
            return 1
        print(pointer_check.stdout.strip())
        # A local record completed inside a function must use the same tag as
        # an earlier declaration required by static storage or generated helpers.
        scope_command = [sys.executable,
                         str(root / "tests/compiler-support/c-record-declaration-scope.py"),
                         "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
                         "--gcc", shutil.which(os.environ.get("GCC") or "gcc"),
                         "--work-dir", str(working)]
        if clang:
            scope_command += ["--clang", clang]
        scope_check = subprocess.run(scope_command, cwd=working, text=True,
                                     capture_output=True, timeout=180, check=False)
        if scope_check.returncode:
            print(scope_check.stdout + scope_check.stderr)
            return 1
        print(scope_check.stdout.strip())
        prototype_command = [sys.executable,
                             str(root / "tests/compiler-support/c-record-prototype-views.py"),
                             "--fbc", str(options.fbc.resolve()), "--prefix", str(toolchain_prefix),
                             "--gcc", shutil.which(os.environ.get("GCC") or "gcc"),
                             "--work-dir", str(working)]
        if clang:
            prototype_command += ["--clang", clang]
        prototype_check = subprocess.run(prototype_command, cwd=working, text=True,
                                         capture_output=True, timeout=180, check=False)
        if prototype_check.returncode:
            print(prototype_check.stdout + prototype_check.stderr)
            return 1
        print(prototype_check.stdout.strip())
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
        # AROS uses emulated TLS. An asm label on an ordinary dollar-mangled
        # name can leak GCC's internal '*' marker into its descriptor name.
        for producer in ("gcc", "clang"):
            generated = working / ("aros-tls-" + producer + ".c")
            command = [*compiler_args, "-r", "-gen", producer, "-target", "aros",
                       str(root / "tests/quirk/c-wince-tls-names.bas"),
                       "-o", str(generated)]
            emitted = subprocess.run(command, cwd=working, text=True,
                                     capture_output=True, timeout=120, check=False)
            if emitted.returncode:
                print(f"TLS/aros/{producer}: emission failed\n{emitted.stdout}{emitted.stderr}")
                return 1
            declarations = [line for line in generated.read_text(encoding="utf-8").splitlines()
                            if "__thread" in line]
            if (len(declarations) != 3 or
                    any("__asm__" in line for line in declarations) or
                    not any("fbcTlsValue$" in line for line in declarations)):
                print(f"TLS/aros/{producer}: incorrect descriptor name declarations")
                return 1
            if clang:
                # No target SDK is needed: emitted C declares its own ABI types.
                compiled = subprocess.run(
                    [clang, "--target=i386-unknown-linux-gnu", "-femulated-tls",
                     "-nostdinc", "-Wall", "-Wextra", "-Werror", "-c", str(generated),
                     "-o", str(working / ("aros-tls-" + producer + ".o"))],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if compiled.returncode:
                    print(f"TLS/aros/{producer}: emulated TLS compilation failed\n"
                          f"{compiled.stdout}{compiled.stderr}")
                    return 1
            print(f"TLS/aros/{producer}: passed")
        if "llvm" in backends:
            if llc is None:
                print("llvm: configured compiler is unavailable")
                return 1
            # Run llc for ARM64 even on an x86 host. IR-only checks would miss
            # an architecture name copied from the GCC command-line interface.
            source = working / "llvm-aarch64.bas"
            source.write_text('function increment( byval value as longint ) as longint\n'
                              '    asm\n        nop\n    end asm\n'
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
                # The C emitter must also preserve Windows' reserved X18.
                # Cross-compile its ASM body so a warning cannot wait until
                # the native ARM64 job to reveal an invalid clobber list.
                emitted = subprocess.run(
                    [*compiler_args, "-r", "-gen", "gcc", "-target", "win32",
                     "-arch", "aarch64", str(source)], cwd=working, text=True,
                    capture_output=True, timeout=120, check=False)
                if emitted.returncode:
                    print("C/aarch64: emission failed\n" + emitted.stdout + emitted.stderr)
                    return 1
                compiled = subprocess.run(
                    [clang, "--target=aarch64-w64-windows-gnu", "-std=gnu2x",
                     "-Wall", "-Wextra", "-Werror", "-nostdinc", "-c",
                     str(source.with_suffix(".c")), "-o", str(working / "asm-aarch64.o")],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if compiled.returncode:
                    print("C/aarch64: assembly clobbers failed\n" + compiled.stdout + compiled.stderr)
                    return 1
                print("C/aarch64: assembly clobbers passed")
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
                # Static array descriptors can point before their storage.
                # Exercise their signed relocations through LLVM and Clang's
                # COFF assembler even when the native compiler is 64-bit.
                compiled = subprocess.run(
                    [*compiler_args, "-c", "-gen", "llvm", "-target", "win32", "-arch", "686",
                     str(root / "tests/compiler-support/c-opaque-local-metadata.bas"),
                     "-o", str(working / "llvm-win32-metadata.o")],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False,
                    env={**os.environ, "LLC": llc, "CLANG": clang})
                if compiled.returncode:
                    print("LLVM/win32: descriptor relocations failed\n" + compiled.stdout + compiled.stderr)
                    return 1
                print("LLVM/win32: descriptor relocations passed")
        try:
            target = subprocess.run([str(options.fbc.resolve()), "-print", "target"],
                                    cwd=working, text=True, capture_output=True,
                                    timeout=30, check=False)
            if target.returncode:
                print(f"compiler target query failed\n{target.stdout}{target.stderr}")
                return 1
            system, _, architecture = target.stdout.strip().partition("-")
            # The driver's legacy Win64 target spelling omits the CPU suffix.
            # It still needs the x86-64 layout, arithmetic and TLS fixtures.
            if system == "win64" and not architecture:
                architecture = "x86_64"
            if system in ("win32", "win64"):
                sys.path.insert(0, str(root / "tests/compiler-support"))
                from native_c_support import native_path
                printer_exe = working / "win32-printer-geometry.exe"
                printer_arch = (["-m64"] if architecture == "x86_64" else
                                ["-m32"] if architecture in ("", "x86", "686") else [])
                runtime_query = subprocess.run([*compiler_args, "-print", "fblibdir"],
                    cwd=working, text=True, capture_output=True, timeout=30, check=False)
                if runtime_query.returncode:
                    print("Windows runtime directory query failed\n" + runtime_query.stderr)
                    return 1
                printer_runtime_dir = native_path(runtime_query.stdout.strip())
                printer_runtime = printer_runtime_dir / "libfb.a"
                # This C client uses runtime helpers. Initialize their context
                # and TLS through the same startup object as BASIC clients.
                printer_build = subprocess.run(
                    [os.environ.get("GCC") or "gcc", *printer_arch, "-std=gnu2x", "-O2",
                     "-Wall", "-Wextra", "-fno-strict-aliasing", "-ffunction-sections",
                     "-fdata-sections", str(root / "tests/compiler-support/win32-printer-geometry.c"),
                     "-Wl,--gc-sections", str(printer_runtime_dir / "fbrt0.o"), str(printer_runtime),
                     "-lwinspool", "-lgdi32", "-lm", "-mthreads", "-o", str(printer_exe)],
                    cwd=working, text=True, capture_output=True, timeout=120, check=False)
                if printer_build.returncode:
                    print(f"Win32 printer geometry build failed\n{printer_build.stdout}{printer_build.stderr}")
                    return 1
                printer_run = subprocess.run([str(printer_exe)], cwd=working, text=True,
                                             capture_output=True, timeout=30, check=False)
                if printer_run.returncode or "PRINTER_GEOMETRY_PASS" not in printer_run.stdout:
                    print(f"Win32 printer geometry failed (exit {printer_run.returncode})\n{printer_run.stdout}{printer_run.stderr}")
                    return 1
                print("win32/printer-geometry: passed")
                for console_wide in (False, True):
                    console_kind = "wide" if console_wide else "narrow"
                    console_exe = working / f"win32-console-buffer-{console_kind}.exe"
                    console_build = subprocess.run(
                        [os.environ.get("GCC") or "gcc", *printer_arch, "-std=gnu2x", "-O2",
                         "-Wall", "-Wextra", "-fno-strict-aliasing", "-ffunction-sections",
                         "-fdata-sections", *(["-DCONSOLE_BUFFER_WIDE"] if console_wide else []),
                         str(root / "tests/compiler-support/win32-console-buffer.c"),
                         "-Wl,--gc-sections", str(printer_runtime),
                         "-lm", "-mthreads", "-o", str(console_exe)],
                        cwd=working, text=True, capture_output=True, timeout=120, check=False)
                    if console_build.returncode:
                        print(f"Win32 console {console_kind} build failed\n{console_build.stdout}{console_build.stderr}")
                        return 1
                    console_run = subprocess.run([str(console_exe)], cwd=working, text=True,
                                                 capture_output=True, timeout=30, check=False)
                    if console_run.returncode or "CONSOLE_BUFFER_PASS" not in console_run.stdout:
                        print(f"Win32 console {console_kind} failed\n{console_run.stdout}{console_run.stderr}")
                        return 1
                    print(f"win32/console-buffer-{console_kind}: passed")
                variant_command = [sys.executable,
                                   str(root / "tests/compiler-support/variant-native-abi.py"),
                                   "--fbc", str(options.fbc.resolve()),
                                   "--prefix", str(toolchain_prefix),
                                   "--gcc", os.environ.get("GCC") or "gcc",
                                   "--work-dir", str(working)]
                if clang:
                    variant_command += ["--clang", clang]
                variant_check = subprocess.run(variant_command, cwd=working, text=True,
                                               capture_output=True, timeout=180, check=False)
                if variant_check.returncode:
                    print(variant_check.stdout + variant_check.stderr)
                    return 1
                print(variant_check.stdout.strip())
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
            if system == "win64" or (architecture == "x86_64" and system in (
                    "linux", "android", "darwin", "freebsd", "openbsd", "netbsd",
                    "dragonfly", "solaris", "illumos", "haiku")):
                stdio_object = working / "stdio-contract.o"
                c_build = subprocess.run(
                    [os.environ.get("GCC") or "gcc", "-m64", "-c",
                     str(root / "tests/compiler-support/stdio-contract.c"),
                     "-o", str(stdio_object)], cwd=working, text=True,
                    capture_output=True, timeout=60, check=False)
                if c_build.returncode:
                    print("stdio contract fixture compilation failed\n"
                          f"{c_build.stdout}{c_build.stderr}")
                    return 1
                fixtures.append(("stdio-contract", "tests/compiler-support/stdio-contract.bas", ""))
                if system == "win64":
                    # Compare opaque UCRT layouts when exposed by the SDK,
                    # then check stream identity and I/O with either CRT.
                    crt_object = working / "crt-native-values.o"
                    c_build = subprocess.run(
                        [os.environ.get("GCC") or "gcc", "-m64", "-std=c99",
                         "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-c",
                         str(root / "tests/compiler-support/crt-native-values.c"),
                         "-o", str(crt_object)], cwd=working, text=True,
                        capture_output=True, timeout=60, check=False)
                    if c_build.returncode:
                        print("CRT native observer compilation failed\n"
                              f"{c_build.stdout}{c_build.stderr}")
                        return 1
                    fixtures.append(("crt-native-values",
                                     "tests/compiler-support/crt-native-values.bas",
                                     "CRT_NATIVE_VALUES_PASS"))
                if system != "android":
                    wide_object = working / "wide-libc-native.o"
                    c_build = subprocess.run(
                        [os.environ.get("GCC") or "gcc", "-m64", "-c",
                         str(root / "tests/compiler-support/c-wide-libc-native.c"),
                         "-o", str(wide_object)], cwd=working, text=True,
                        capture_output=True, timeout=60, check=False)
                    if c_build.returncode:
                        print("wide libc fixture compilation failed\n"
                              f"{c_build.stdout}{c_build.stderr}")
                        return 1
                    fixtures.append(("wide-libc", "tests/compiler-support/c-wide-libc.bas", ""))
                    fixtures.append(("long-literals", "tests/compiler-support/c-long-literals.bas", ""))
                    address_object = working / "procedure-address-native.o"
                    c_build = subprocess.run(
                        [os.environ.get("GCC") or "gcc", "-m64", "-c",
                         str(root / "tests/compiler-support/procedure-address-identity.c"),
                         "-o", str(address_object)], cwd=working, text=True,
                        capture_output=True, timeout=60, check=False)
                    if c_build.returncode:
                        print("procedure address fixture compilation failed\n"
                              f"{c_build.stdout}{c_build.stderr}")
                        return 1
                    fixtures.append(("procedure-address-identity", "tests/compiler-support/procedure-address-identity.bas", ""))
                    fixtures.append(("deep-reachability", "tests/compiler-support/c-procedure-deep-reachability.bas", "DEEP_REACHABILITY=PASS"))
                    fixtures.append(("array-virtual-origins", "tests/compiler-support/c-array-virtual-origins.bas", "ARRAY_ORIGIN=PASS"))
                    fixtures.append(("destructor-reachability", "tests/compiler-support/c-destructor-reachability.bas", "LEAF= 0\nLEAF= 0\nLEAF= 0\nLEAF= 0\nLEAF= 0\nLEAF= 0\nVIRTUAL_ITEM\nVIRTUAL_BASE\nDESTRUCTOR_ROOTS_MAIN=PASS\nSTATIC_ROOT= 17\nLEAF= 0\nLEAF= 0"))
            if architecture in ("x86_64", "aarch64"):
                fixtures.extend((
                    ("int128-layout", "tests/compiler-support/integer128-layout.bas",
                     "128-bit integer layout passed"),
                    ("int128-arithmetic", "tests/compiler-support/integer128-arithmetic.bas",
                     "128-bit integer arithmetic passed"),
                    ("thread-local", "tests/compiler-support/thread-local.bas",
                     "thread-local storage passed"),
                    ("thread-local-contextual-procedure", "tests/compiler-support/thread-contextual-procedure.bas",
                     "CONTEXTUAL_THREAD_PROCEDURE_PASS"),
                    ("thread-local-contextual-namespace", "tests/compiler-support/thread-contextual-namespace.bas",
                     "CONTEXTUAL_THREAD_NAMESPACE_PASS"),
                    ("thread-local-contextual-import", "tests/compiler-support/thread-contextual-import.bas",
                     "CONTEXTUAL_THREAD_IMPORT_PASS"),
                ))
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"C ABI fixture: {error}")
            return 1
        for backend in backends:
            for fixture_index, (name, source, expected) in enumerate(fixtures):
                if name in ("wide-libc", "long-literals", "procedure-address-identity", "deep-reachability", "destructor-reachability", "array-virtual-origins") and backend not in ("gcc", "clang"):
                    continue
                if name in ("gosub-values", "label-flag-layout") and backend not in ("gcc", "clang"):
                    continue
                if name == "asm-att-llvm" and backend != "llvm":
                    continue
                if name == "asm-att-llvm" and architecture not in ("x86", "686", "x86_64", ""):
                    print(f"{backend}/{name}: x86 assembly fixture is unavailable on {architecture}")
                    continue
                # Windows can classify an unmanifested 32-bit executable as
                # an installer when its name contains "patch", including
                # "dispatch". These are ordinary test programs.
                executable = working / (backend + "-test-" + str(fixture_index))
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
                if name == "custom-entry":
                    command += ["-entry", "custom_entry"]
                if name == "label-flag-layout":
                    command += ["-i", str(root / "src/compiler")]
                if name == "asm-att-llvm":
                    command += ["-asm", "att"]
                if name == "qb-headers":
                    command += ["-lang", "qb"]
                if name in ("dispatch-values", "gosub-values", "error-capture-layout"):
                    # RESUME needs the failing statement's continuation labels.
                    command += ["-ex"]
                if name == "terminating-context":
                    command += ["-exx"]
                if name == "virtual-receiver-checked":
                    command += ["-exx"]
                if name.startswith("thread-local"):
                    command += ["-mt"]
                if name == "startup-declarations":
                    module_object = working / (backend + "-startup-module.o")
                    module_build = subprocess.run(
                        [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                         "-d", "STARTUP_MODULE_PROBE", "-m", "startup_module_driver",
                         "-c", str(input_source), "-o", str(module_object)],
                        cwd=working, text=True, capture_output=True, timeout=600, check=False)
                    if module_build.returncode:
                        print(f"{backend}/{name}: module failed\n"
                              f"{module_build.stdout}{module_build.stderr}")
                        return 1
                    command += [str(module_object)]
                if name == "external-prototypes":
                    observer = working / (backend + "-external-prototypes.o")
                    built = subprocess.run(
                        [os.environ.get("GCC") or "gcc", "-std=c99", "-Wall", "-Wextra",
                         "-Werror", "-c", str(root / "tests/compiler-support/c-external-prototypes.c"),
                         "-o", str(observer)], cwd=working, text=True, capture_output=True,
                        timeout=60, check=False)
                    if built.returncode:
                        print(f"{backend}/{name}: C observer failed\n{built.stdout}{built.stderr}")
                        return 1
                    command += [str(observer)]
                if name == "thread-local-extern":
                    command += [str(thread_local_extern_object)]
                if name == "incomplete-extern":
                    command += [str(incomplete_extern_object)]
                    if backend in ("gcc", "clang"):
                        # An address-only object must retain its unknown extent.
                        # A char surrogate happens to link, but gives C the wrong
                        # bounds when the address identifies a larger region.
                        generated = working / (backend + "-incomplete-extern.c")
                        emitted = subprocess.run(
                            [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                             "-r", str(input_source), "-o", str(generated)],
                            cwd=working, text=True, capture_output=True, timeout=120,
                            check=False)
                        if emitted.returncode or not re.search(
                                r"\bextern\s+struct\s+\w+\s+c2fb_incomplete_object\s*;",
                                generated.read_text() if generated.is_file() else ""):
                            print(f"{backend}/{name}: incomplete object extent lost\n"
                                  f"{emitted.stdout}{emitted.stderr}")
                            return 1
                if name == "long-literals":
                    generated = working / (backend + "-long-literals.c")
                    emitted = subprocess.run(
                        [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                         "-m", input_source.stem, "-r", str(input_source), "-o", str(generated)],
                        cwd=working, text=True, capture_output=True, timeout=120,
                        check=False)
                    if emitted.returncode:
                        print(f"{backend}/{name}: literal emission failed\n"
                              f"{emitted.stdout}{emitted.stderr}")
                        return 1
                    checked = subprocess.run(
                        [os.environ.get("GCC") or "gcc", "-std=gnu11", "-nostdinc",
                         "-Wpedantic", "-Werror=overlength-strings", "-fsyntax-only", str(generated)],
                        cwd=working, text=True, capture_output=True, timeout=120,
                        check=False)
                    if checked.returncode:
                        print(f"{backend}/{name}: long quoted literal remains\n"
                              f"{checked.stdout}{checked.stderr}")
                        return 1
                if name == "deep-reachability":
                    generated = working / (backend + "-deep-reachability.c")
                    emitted = subprocess.run(
                        [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                         "-m", input_source.stem, "-r", str(input_source), "-o", str(generated)],
                        cwd=working, text=True, capture_output=True, timeout=120,
                        check=False)
                    if emitted.returncode or "UNUSED_DEEP_LEAF" in generated.read_text().upper():
                        print(f"{backend}/{name}: unreachable private body remains\n"
                              f"{emitted.stdout}{emitted.stderr}")
                        return 1
                if name == "destructor-reachability":
                    generated = working / (backend + "-destructor-reachability.c")
                    emitted = subprocess.run(
                        [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                         "-g", "-m", input_source.stem, "-r", str(input_source), "-o", str(generated)],
                        cwd=working, text=True, capture_output=True, timeout=120,
                        check=False)
                    if emitted.returncode or "UNOBSERVED_DTOR" in generated.read_text().upper():
                        print(f"{backend}/{name}: unreachable destructor remains\n"
                              f"{emitted.stdout}{emitted.stderr}")
                        return 1
                if name in ("array-virtual-origins", "pointer-values") and backend == "clang":
                    command += ["-Wc", "-fsanitize=pointer-overflow",
                                "-Wc", "-fsanitize-trap=pointer-overflow"]
                if name == "procedure-address-identity":
                    command += [str(address_object)]
                if name == "stdio-contract":
                    command += [str(stdio_object)]
                if name == "crt-native-values":
                    command += [str(crt_object)]
                if name == "wide-libc":
                    command += [str(wide_object)]
                    generated = working / (backend + "-wide-libc.c")
                    emitted = subprocess.run(
                        [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                         "-m", input_source.stem, "-r", str(input_source), "-o", str(generated)],
                        cwd=working, text=True, capture_output=True, timeout=120,
                        check=False)
                    code = generated.read_text() if generated.is_file() else ""
                    if (emitted.returncode or "fb_c_wcslen(" not in code or "fb_c_wcsncmp(" not in code or
                            "fb_c_wcschr(" not in code or "&wcschr" not in code or "&fb_c_wcschr" in code or
                            "&wcslen" not in code or "&fb_c_wcslen" in code):
                        print(f"{backend}/{name}: wide alignment/address contract changed\n"
                              f"{emitted.stdout}{emitted.stderr}")
                        return 1
                if name == "readonly-entries" and system == "win64" and backend in ("gcc", "clang"):
                    # Private CDECL bodies bypass const inference. This host's
                    # BASIC convention exercises the separate implementation.
                    generated = working / (backend + "-readonly-entries.c")
                    emitted = subprocess.run(
                        [*compiler_args, "-i", str(root / "inc"), "-gen", backend,
                         "-m", input_source.stem, "-r", str(input_source), "-o", str(generated)],
                        cwd=working, text=True, capture_output=True, timeout=120,
                        check=False)
                    code = generated.read_text() if generated.is_file() else ""
                    if (emitted.returncode or "DIRECTREAD" in code or "DEADADDRESS" in code or
                            any(token not in code for token in
                                ("ADDRESSREAD", "SELFADDRESSREAD", "readonly_shared_entry"))):
                        print(f"{backend}/{name}: private entry reachability changed\n"
                              f"{emitted.stdout}{emitted.stderr}")
                        return 1
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
                    if backend in ("gcc", "clang"):
                        diagnostic = built.stdout + built.stderr
                        disabled = ("-Wno-main", "-Wno-misleading-indentation",
                                    "-Wno-builtin-requires-header", "-Wno-uninitialized",
                                    "-Wno-warn-absolute-paths")
                        if "-Wall" not in diagnostic or any(flag in diagnostic for flag in disabled):
                            print(f"{backend}/{name}: C warnings were disabled\n{diagnostic}")
                            return 1
                    arguments = [str(executable)]
                    if name == "stdio-contract":
                        arguments.append(str(working / "stdio-roundtrip.txt"))
                    if name == "crt-native-values":
                        arguments.append(str(working / "crt-values.bin"))
                    if name == "fbcunit-report":
                        arguments += [str(working / filename) for filename in
                                      ("report-first.txt", "report-next.txt", "report.xml")]
                    executed = subprocess.run(arguments, cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"{backend}/{name}: {error}")
                    return 1
                matches_expected = executed.stdout.strip() == expected
                if name == "label-flag-layout":
                    fields = executed.stdout.split()
                    matches_expected = (len(fields) == 5 and fields[0] == expected
                                        and all(field.isdecimal() for field in fields[1:]))
                if executed.returncode or not matches_expected:
                    print(f"{backend}/{name}: execution failed (exit {executed.returncode})\n{executed.stdout}{executed.stderr}")
                    return 1
                print(f"{backend}/{name}: passed")
                if name == "virtual-receiver-checked":
                    for mode in ("null-virtual", "null-abstract"):
                        try:
                            rejected = subprocess.run(
                                [str(executable), mode], cwd=working, text=True,
                                capture_output=True, timeout=30, check=False)
                        except (OSError, subprocess.TimeoutExpired) as error:
                            print(f"{backend}/{name}/{mode}: {error}")
                            return 1
                        diagnostic = rejected.stdout + rejected.stderr
                        if rejected.returncode != 7 or "null pointer access" not in diagnostic.lower():
                            print(f"{backend}/{name}/{mode}: receiver check failed\n{diagnostic}")
                            return 1
                        print(f"{backend}/{name}/{mode}: passed")

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
            if check_i386_clongdouble_abi(
                    root, options.fbc.resolve(), working, clang, llc) != 0:
                return 1

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
            arithmetic_object = working / "gas-int128-arithmetic.o"
            # MinGW's native assembler produces COFF and cannot consume the
            # ELF directives selected by linux-x86. Use an explicit ELF target.
            if clang is not None:
                assemble_command = [clang, "-target", "i386-linux-gnu", "-c", "-x", "assembler",
                                    str(assembly), "-o", str(arithmetic_object)]
            elif system == "linux" and architecture in ("x86", "686", "x86_64"):
                assemble_command = [os.environ.get("AS") or "as", "--32", "-o",
                                    str(arithmetic_object), str(assembly)]
            else:
                assemble_command = None
            if assemble_command is not None:
                assembled = subprocess.run(assemble_command, cwd=working, text=True,
                                           capture_output=True, timeout=60, check=False)
                if assembled.returncode:
                    print(f"gas/int128-arithmetic: assembly failed\n{assembled.stdout}{assembled.stderr}")
                    return 1
                header = arithmetic_object.read_bytes()[:20]
                if header[:5] != b"\x7fELF\x01" or header[18:20] != b"\x03\x00":
                    print("gas/int128-arithmetic: expected an i386 ELF32 object")
                    return 1
                print("gas/int128-arithmetic: ELF32 object passed")
            else:
                print("gas/int128-arithmetic: emitted; ELF32 assembly requires Clang or AS on Windows")

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

            # Keyword lookup is shared by the C and native backends. Keep the
            # ordinary name and storage-prefix contracts covered on both paths.
            for role in ("procedure", "namespace", "import"):
                source = root / ("tests/compiler-support/thread-contextual-" + role + ".bas")
                executable = working / ("gas64-thread-contextual-" + role)
                command = [*compiler_args, "-i", str(root / "inc"),
                           "-gen", "gas64", "-mt", str(source), "-x", str(executable)]
                try:
                    built = subprocess.run(command, cwd=working, text=True,
                                           capture_output=True, timeout=120, check=False)
                    if built.returncode:
                        print(f"gas64/thread-contextual-{role}: compilation failed\n"
                              f"{built.stdout}{built.stderr}")
                        return 1
                    executed = subprocess.run([str(executable)], cwd=working, text=True,
                                              capture_output=True, timeout=30, check=False)
                except (OSError, subprocess.TimeoutExpired) as error:
                    print(f"gas64/thread-contextual-{role}: {error}")
                    return 1
                if (executed.returncode or executed.stdout.strip() !=
                        "CONTEXTUAL_THREAD_" + role.upper() + "_PASS"):
                    print(f"gas64/thread-contextual-{role}: execution failed\n"
                          f"{executed.stdout}{executed.stderr}")
                    return 1
                print(f"gas64/thread-contextual-{role}: passed")

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
