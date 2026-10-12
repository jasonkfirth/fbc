#!/usr/bin/env python3
"""
FreeBASIC serialization regression runner
File: test-serialization.py
Purpose: Qualify generated traversal, runtime failures and target interoperability.
Responsibilities: Isolated builds, independent wire fixtures and rejected programs.
This file intentionally contains no production serializer or compiler build logic.
"""

import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile


def run(arguments, working, environment=None, timeout=120):
    result = subprocess.run([str(arg) for arg in arguments], cwd=working,
                            env=environment, capture_output=True, text=True,
                            timeout=timeout, check=False)
    if result.returncode:
        raise RuntimeError(f"exit {result.returncode}: {arguments}\n{result.stdout}{result.stderr}")
    return result.stdout


def golden_payload():
    # Construct the wire bytes independently of runtime output, including its
    # canonical schema. Descriptor size, native INTEGER width and padding are
    # deliberately absent from this reference.
    schema = (b"C{LEADING_BYTE=U1;TEXT_VALUE=S;NUMBER=N;UNSIGNED_NUMBER=V;RATIO=D8;"
              b"VALUES=R2[C{TEXT_VALUE=S;NUMBER=N;}];"
              b"FIXED_VALUES=A1[-1:1;C{TEXT_VALUE=S;NUMBER=N;}];FLAG=B;}")

    def string(value):
        return struct.pack("<I", len(value)) + value

    body = bytes([250]) + string(b"portable\0payload")
    body += struct.pack("<qQd", -2147483647, 4294967295, 0.125)
    body += bytes([2]) + struct.pack("<qqqq", -2, 0, 3, 4)
    body += (string(b"") + struct.pack("<q", 0)) * 5
    body += string(b"owned") + struct.pack("<q", -12345)
    body += string(b"inline") + struct.pack("<q", 0)
    body += (string(b"") + struct.pack("<q", 0)) * 2 + bytes([1])
    return b"FBS1" + struct.pack("<I", len(schema)) + schema + body


def rejected_sources():
    values = {
        "pointer": "dim value as integer ptr\ndim payload as string = Serialize(value)",
        "pointer-member": "type Bad\n p as integer ptr\nend type\ndim value as Bad\ndim payload as string = Serialize(value)",
        "procedure-pointer": "dim value as sub()\ndim payload as string = Serialize(value)",
        "union": "union Bad\n a as long\n b as single\nend union\ndim value as Bad\ndim payload as string = Serialize(value)",
        "union-member": "type Bad\n union\n a as long\n b as single\n end union\nend type\ndim value as Bad\ndim payload as string = Serialize(value)",
        "bitfield": "type Bad\n a:1 as ulong\nend type\ndim value as Bad\ndim payload as string = Serialize(value)",
        "bitfield-value": "type Bad\n a:1 as ulong\nend type\ndim value as Bad\ndim payload as string = Serialize(value.a)",
        "constructor": "type Bad\n declare constructor()\n a as long\nend type\nconstructor Bad()\nend constructor\ndim value as Bad\ndim payload as string = Serialize(value)",
        "destructor": "type Bad\n declare destructor()\n a as long\nend type\ndestructor Bad()\nend destructor\ndim value as Bad\ndim payload as string = Serialize(value)",
        "private-field": "type Bad\n private:\n a as long\nend type\ndim value as Bad\ndim payload as string = Serialize(value)",
        "zstring": "dim value as zstring * 5\ndim payload as string = Serialize(value)",
        "wstring": "dim value as wstring * 5\ndim payload as string = Serialize(value)",
        "constant-destination": "dim value as const long = 7\nprint Deserialize(Serialize(value), value)",
        "constant-field": "type Bad\n value as const long\nend type\ndim value as Bad\nprint Deserialize(Serialize(value), value)",
        "temporary-destination": "function make_value() as string\n return \"x\"\nend function\ndim value as string\nprint Deserialize(Serialize(value), make_value())",
    }
    declarations = ["type Level0\n value as long\nend type"]
    for depth in range(1, 34):
        declarations.append(f"type Level{depth}\n value as Level{depth-1}\nend type")
    values["depth"] = "\n".join(declarations) + "\ndim value as Level33\ndim payload as string = Serialize(value)"
    values["nodes"] = "type Bad\n" + "\n".join(f" value{i} as long" for i in range(4096)) + "\nend type\ndim value as Bad\ndim payload as string = Serialize(value)"
    return values


def compatibility_source():
    return """' Existing programs retain these ordinary identifiers and operations.
function Serialize(byval value as long) as long
    return value + 1
end function
function Deserialize(byval value as long) as long
    return value - 1
end function
type ExistingState
    text_value as string
    values(any) as long
end type
dim as ExistingState original, copied
original.text_value = "original"
redim original.values(-1 to 1)
original.values(0) = 42
copied = original
original.text_value = "changed"
original.values(0) = 99
if copied.text_value <> "original" or copied.values(0) <> 42 then end 1
if Serialize(41) <> 42 or Deserialize(43) <> 42 then end 2
dim as long written = -12345, restored
open "compatibility.bin" for binary as #1
put #1, , written
get #1, 1, restored
close #1
if restored <> written then end 3
kill "compatibility.bin"
print "SERIALIZATION_EXISTING_BEHAVIOR=PASS"
"""


def test_target(root, compiler, cc, prefix, target_flags, working, backends, tag):
    environment = os.environ.copy()
    environment["PATH"] = str(cc.parent) + os.pathsep + environment.get("PATH", "")
    if os.name == "nt":
        # Explicit tool overrides take precedence over -buildprefix. Do not
        # let an inherited Win64 LD/AS override escape into the Win32 lane.
        for variable, name in (("GCC", "gcc"), ("AS", "as"), ("LD", "ld"), ("AR", "ar")):
            tool = cc.parent / (name + ".exe")
            if not tool.is_file():
                raise RuntimeError(f"Missing target tool: {tool}")
            environment[variable] = str(tool)
    flags = [compiler, "-prefix", prefix, "-i", root / "inc"] + target_flags
    libdir = Path(run(flags + ["-print", "fblibdir"], working, environment).strip())
    if not (libdir / "libfb.a").exists():
        # Old standalone FreeBASIC distributions use lib/<target> rather than
        # lib/freebasic/<target>. This is an explicit SDK compatibility path.
        target = "win32" if tag == "32" else libdir.name
        alternative = prefix / "lib" / target
        if not (alternative / "libfb.a").exists():
            raise RuntimeError(f"Missing runtime archive: {libdir}")
        libdir = alternative
        flags += ["-p", libdir]

    includes = ["-I", root / "src/rtlib"]
    warning_flags = ["-Wall", "-Wextra", "-Werror"]
    codec = working / f"codec-{tag}.o"
    run([cc] + warning_flags + includes + ["-c", root / "src/rtlib/serialization.c", "-o", codec], working, environment)
    fault_codec = working / f"fault-codec-{tag}.o"
    run([cc] + warning_flags + includes + ["-Dcalloc=ser_test_calloc", "-Drealloc=ser_test_realloc",
        "-Dfree=ser_test_free", "-c", root / "src/rtlib/serialization.c", "-o", fault_codec], working, environment)
    runtime_test = working / f"runtime-{tag}.exe"
    # The direct C link pulls in FreeBASIC's runtime initialization, including
    # Unix terminal services. Match the runtime's ncurses dependency even
    # though the harness uses C stdio for its own output.
    system_flags = [] if os.name == "nt" else ["-lm", "-pthread", "-lncurses"]
    runtime_objects = [libdir / "fbrt0.o"] if (libdir / "fbrt0.o").exists() else []
    run([cc] + warning_flags + includes + [root / "tests/serialization/runtime.c", fault_codec] +
        runtime_objects + [libdir / "libfb.a"] + system_flags + ["-o", runtime_test], working, environment)
    output = run([runtime_test], working, environment)
    if "SERIALIZATION_RUNTIME=PASS" not in output:
        raise RuntimeError(output)
    print(output.strip())

    interop = None
    for backend in backends:
        executable = working / f"roundtrip-{tag}-{backend}.exe"
        native_flags = flags + ["-gen", backend, "-g", "-exx"]
        run(native_flags + [root / "tests/serialization/roundtrip.bas", codec, "-x", executable], working, environment)
        output = run([executable], working, environment)
        if "SERIALIZATION_ROUNDTRIP=PASS" not in output:
            raise RuntimeError(output)
        print(f"SERIALIZATION_BACKEND=PASS TARGET={tag} BACKEND={backend}")
        # Explicit objects allow qualification before an installed runtime
        # archive is replaced. The native build also checks archive integration.
        if backend == backends[0]:
            interop = working / f"interop-{tag}.exe"
            run(native_flags + [root / "tests/serialization/interop.bas", codec, "-x", interop], working, environment)
            if tag == "native":
                archive_test = working / "archive-roundtrip.exe"
                run(native_flags + [root / "tests/serialization/roundtrip.bas", "-x", archive_test], working, environment)
                if "SERIALIZATION_ROUNDTRIP=PASS" not in run([archive_test], working, environment):
                    raise RuntimeError("Native runtime archive integration failed")
                print("SERIALIZATION_RUNTIME_ARCHIVE=PASS")
            multithreaded = working / f"roundtrip-mt-{tag}.exe"
            run(native_flags + ["-mt", root / "tests/serialization/roundtrip.bas", codec,
                                "-x", multithreaded], working, environment)
            if "SERIALIZATION_ROUNDTRIP=PASS" not in run([multithreaded], working, environment):
                raise RuntimeError("Multithreaded runtime integration failed")
            print(f"SERIALIZATION_MT_RUNTIME=PASS TARGET={tag}")
        existing_source = working / "existing-behavior.bas"
        existing_source.write_text(compatibility_source(), encoding="utf-8")
        existing = working / f"existing-{tag}-{backend}.exe"
        run(native_flags + [existing_source, "-x", existing], working, environment)
        if "SERIALIZATION_EXISTING_BEHAVIOR=PASS" not in run([existing], working, environment):
            raise RuntimeError("Existing identifier, assignment or file I/O behavior changed")
        for name, body in rejected_sources().items():
            source = working / f"invalid-{name}.bas"
            source.write_text("#include once \"fbserialize.bi\"\n" + body + "\n", encoding="utf-8")
            result = subprocess.run([str(arg) for arg in native_flags + ["-c", source, "-o", working / "invalid.o"]],
                                    cwd=working, env=environment, capture_output=True, text=True, timeout=120, check=False)
            diagnostic = result.stdout + result.stderr
            expected = "Cannot modify a constant" if name == "constant-destination" else "serialization:"
            if result.returncode == 0 or expected.lower() not in diagnostic.lower():
                raise RuntimeError(f"{backend}/{name}: missing intended rejection\n{diagnostic}")
        print(f"SERIALIZATION_DIAGNOSTICS=PASS TARGET={tag} BACKEND={backend} CASES={len(rejected_sources())}")
    return interop, environment


def test_ubsan(root, clang, cc, libdir, working, environment):
    # Trap mode needs no sanitizer runtime installation. It checks the codec
    # object, including packed descriptors and extreme malformed bounds, while
    # linking the real FreeBASIC runtime services used by the fault harness.
    codec = working / "codec-ubsan.o"
    includes = ["-I", root / "src/rtlib"]
    warnings = ["-Wall", "-Wextra", "-Werror"]
    run([clang] + warnings + includes + ["-fsanitize=undefined", "-fsanitize-trap=all",
        "-Dcalloc=ser_test_calloc", "-Drealloc=ser_test_realloc", "-Dfree=ser_test_free",
        "-c", root / "src/rtlib/serialization.c", "-o", codec], working, environment)
    runtime_objects = [libdir / "fbrt0.o"] if (libdir / "fbrt0.o").exists() else []
    system_flags = [] if os.name == "nt" else ["-lm", "-pthread", "-lncurses"]
    executable = working / "runtime-ubsan.exe"
    run([cc] + warnings + includes + [root / "tests/serialization/runtime.c", codec] +
        runtime_objects + [libdir / "libfb.a"] + system_flags + ["-o", executable], working, environment)
    if "SERIALIZATION_RUNTIME=PASS" not in run([executable], working, environment):
        raise RuntimeError("Undefined-behavior sanitizer check failed")
    print("SERIALIZATION_UBSAN=PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--cc", type=Path, default=Path(shutil.which("gcc") or "gcc"))
    parser.add_argument("--backend", action="append", choices=("gcc", "gas64", "gas", "llvm", "clang"))
    parser.add_argument("--cc32", type=Path)
    parser.add_argument("--prefix32", type=Path)
    parser.add_argument("--clang", type=Path, help="Also run the runtime fault harness with undefined-behavior traps")
    parser.add_argument("--baseline", type=Path, help="Compare existing behavior with this unchanged compiler")
    options = parser.parse_args()
    root = options.root.resolve()
    with tempfile.TemporaryDirectory(prefix="fbc-serialization-") as temporary:
        working = Path(temporary)
        compiler = options.fbc.resolve()
        native, environment = test_target(root, compiler, options.cc.resolve(), root, [], working,
                                          options.backend or ["gcc"], "native")
        native_payload = working / "native.bin"
        print(run([native, "write", native_payload], working, environment).strip())
        run([native, "read", native_payload], working, environment)
        if native_payload.read_bytes() != golden_payload():
            raise RuntimeError("Native output differs from independent little-endian wire fixture")
        print("SERIALIZATION_GOLDEN_WIRE=PASS")
        if options.baseline:
            baseline = working / "existing-baseline.exe"
            run([options.baseline.resolve(), "-prefix", root, "-gen", "gcc",
                 working / "existing-behavior.bas", "-x", baseline], working, environment)
            if "SERIALIZATION_EXISTING_BEHAVIOR=PASS" not in run([baseline], working, environment):
                raise RuntimeError("Baseline behavior comparison failed")
            print("SERIALIZATION_BASELINE_PARITY=PASS")
        if options.clang:
            libdir = Path(run([compiler, "-prefix", root, "-print", "fblibdir"], working, environment).strip())
            test_ubsan(root, options.clang.resolve(), options.cc.resolve(), libdir, working, environment)
        if options.cc32 or options.prefix32:
            if not options.cc32 or not options.prefix32:
                parser.error("--cc32 and --prefix32 must be supplied together")
            cc32 = options.cc32.resolve()
            target_flags = ["-target", "win32", "-buildprefix", str(cc32.parent) + "/"]
            target32, environment32 = test_target(root, compiler, cc32, options.prefix32.resolve(),
                target_flags, working, ["gcc", "gas"], "32")
            print(run([target32, "read", native_payload], working, environment32).strip())
            payload32 = working / "32.bin"
            print(run([target32, "write", payload32], working, environment32).strip())
            print(run([native, "read", payload32], working, environment).strip())
            if payload32.read_bytes() != golden_payload():
                raise RuntimeError("32-bit output differs from independent wire fixture")
            print("SERIALIZATION_32_64_EXCHANGE=PASS")
        else:
            print("SERIALIZATION_32_64_EXCHANGE=SKIPPED (no 32-bit SDK supplied)")
    print("SERIALIZATION_REGRESSION=PASS")


if __name__ == "__main__":
    main()

# end of test-serialization.py
