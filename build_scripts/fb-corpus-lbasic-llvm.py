#!/usr/bin/env python3
"""FreeBASIC browser corpus. Build the LLVM 14 libraries for L-BASIC.

fb-corpus-lbasic-llvm.py builds a native TableGen and uses it to generate
headers for the Wasm libraries. Links those libraries with the LLVM C ABI
probe or with the staged L-BASIC emitter. The frontend staging driver owns
the source adaptation and generated parser tables.
"""

import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import runpy
import shutil
import subprocess


OPTIONS = {
    "LLVM_TARGETS_TO_BUILD": "WebAssembly",
    "LLVM_INCLUDE_TESTS": "OFF",
    "LLVM_INCLUDE_EXAMPLES": "OFF",
    "LLVM_INCLUDE_BENCHMARKS": "OFF",
    "LLVM_INCLUDE_DOCS": "OFF",
    "LLVM_INCLUDE_UTILS": "OFF",
    "LLVM_ENABLE_BINDINGS": "OFF",
    "LLVM_ENABLE_ZLIB": "OFF",
    "LLVM_ENABLE_LIBXML2": "OFF",
    "LLVM_ENABLE_TERMINFO": "OFF",
    "LLVM_ENABLE_LIBEDIT": "OFF",
    "LLVM_ENABLE_ASSERTIONS": "OFF",
    "LLVM_BUILD_TOOLS": "OFF",
}


def run(command, work, log_name, env):
    log_path = work / log_name
    print(f"Building LLVM. Log {log_path}", flush=True)
    with log_path.open("wb") as log:
        result = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print("\n".join(log_path.read_text(encoding="utf-8", errors="replace").splitlines()[-30:]))
        raise SystemExit(result.returncode)


def emscripten_environment(toolchain, sdk):
    driver = runpy.run_path(str(Path(__file__).with_name("fb-corpus-js.py")))
    env = driver["configure_tools"](toolchain)
    env["PATH"] = str(sdk / "bin") + os.pathsep + env["PATH"]
    env["EMSDK_PYTHON"] = str(sdk / "bin/python.exe")
    return env


def compile_object(source, output, compiler, env, dialect=None):
    output.mkdir(parents=True, exist_ok=True)
    lowered = output / (source.stem + ".c")
    obj = output / (source.stem + ".o")
    flags = ["-lang", dialect] if dialect else []
    run([str(compiler), *flags, "-r", "-m", source.stem, "-o", str(lowered), str(source)], output, "fbc.log", env)
    run([env["EMCC"], "-c", str(lowered), "-o", str(obj)], output, "emcc.log", env)
    return obj


def link_browser_app(work, obj, compiler, sdk, env, foundation=None):
    library = work / "wasm/lib"
    components = sorted(library.glob("libLLVM*.a"))
    needed = {"libLLVMCore.a", "libLLVMAnalysis.a", "libLLVMBitWriter.a", "libLLVMTarget.a"}
    if not needed.issubset(component.name for component in components):
        raise ValueError("Build the LLVM Wasm libraries before linking the browser app")
    runtime = compiler.parent.parent / "lib/freebasic-js/js-asmjs"
    assets = ["--embed-file", f"{foundation}@/runtime/foundation.a"] if foundation else []
    # Use the C++ driver for LLVM's libc++ dependencies. The FreeBASIC output
    # was compiled as C in the preceding stage, preserving its C ABI.
    command = [str(sdk.parent / "cmd-shims/em++.cmd"), str(obj),
               *(str(component) for component in components), str(runtime / "libfb.a"),
               "--shell-file", str(runtime / "fb_shell.html"),
               "--extern-pre-js", str(runtime / "fb_rtlib.js"),
               "--extern-pre-js", str(runtime / "termlib_min.js"),
               "-sEXPORTED_RUNTIME_METHODS=FS", "-sEXIT_RUNTIME=1", "-sSTACK_SIZE=2MB", "-sALLOW_MEMORY_GROWTH=1",
               *assets, "-o", str(obj.parent / "index.html")]
    run(command, obj.parent, "link.log", env)


def check_emitted_programs(output, foundation, env):
    node = shutil.which("node", path=env["PATH"])
    if not node:
        raise ValueError("Node is required to run the emitted programs")
    results = json.loads((output / "browser-bitcode-results.json").read_text(encoding="utf-8"))
    checks = [row for row in results if "stdout" in row]
    if not checks:
        raise ValueError("Run the browser emitter check before testing its bitcode")
    for row in checks:
        name = row["output"]
        if Path(name).name != name or not name.endswith(".bc"):
            raise ValueError(f"Invalid emitted bitcode name: {name}")
        bitcode = output / name
        if sha256(bitcode.read_bytes()).hexdigest() != row["sha256"]:
            raise ValueError(f"The bitcode changed after the browser check: {name}")
        script = bitcode.with_suffix(".js")
        run([env["EMCC"], str(bitcode), str(foundation), "-sENVIRONMENT=node", "-sEXIT_RUNTIME=1",
             "-o", str(script)], output, bitcode.stem + "-link.log", env)
        stdout_path = bitcode.stem + "-stdout.log"
        run([node, str(script)], output, stdout_path, env)
        actual = (output / stdout_path).read_text(encoding="utf-8")
        expected = row["stdout"]
        matches = actual == expected if row.get("exact") else actual.split() == expected.split()
        if not matches:
            raise ValueError(f"{name} printed {actual!r}, expected {row['stdout']!r}")
        print(f"LBASIC_BROWSER_BITCODE_EXECUTED {name} stdout={actual.strip()!r}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("work", type=Path)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--ninja", type=Path)
    parser.add_argument("--compiler", type=Path)
    parser.add_argument("--frontend", type=Path, help="L-BASIC directory staged with --llvm")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--phase", choices=("host", "wasm", "probe", "emitter-object", "emitter", "execute"), required=True)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    work = args.work.resolve()
    sdk = args.toolchain.resolve() / "toolchain/ucrt64"
    if args.phase == "execute":
        foundation = args.frontend.resolve() / "runtime/foundation.a" if args.frontend else None
        if not foundation or not foundation.is_file():
            parser.error("Testing emitted programs requires the staged L-BASIC directory as --frontend")
        check_emitted_programs(work / "emitter", foundation, emscripten_environment(args.toolchain, sdk))
        return
    if args.phase in ("probe", "emitter-object", "emitter"):
        if args.compiler is None or not args.compiler.is_file():
            parser.error("The browser build requires --compiler pointing to fbc-js.exe")
        compiler = args.compiler.resolve()
        env = emscripten_environment(args.toolchain, sdk)
        if args.phase == "probe":
            source = Path(__file__).with_name("fb-corpus-lbasic-llvm-probe.bas").resolve()
            obj = compile_object(source, work / "probe", compiler, env)
            link_browser_app(work, obj, compiler, sdk, env)
            print("LLVM_WASM_PROBE_LINKED", flush=True)
            return
        if args.frontend is None:
            parser.error("The emitter requires --frontend pointing to the staged L-BASIC directory")
        root = args.frontend.resolve()
        source = root / "lbasic-llvm.bas"
        foundation = root / "runtime/foundation.a"
        if not source.is_file() or not foundation.is_file():
            parser.error("Stage L-BASIC using fb-corpus-lbasic-frontend.py --llvm first")
        obj = compile_object(source, work / "emitter", compiler, env, dialect="fblite")
        if args.phase == "emitter":
            link_browser_app(work, obj, compiler, sdk, env, foundation)
            print("LBASIC_WASM_EMITTER_LINKED", flush=True)
        else:
            print(f"LBASIC_WASM_EMITTER_OBJECT={obj}", flush=True)
        return
    cmake = shutil.which("cmake")
    ninja = args.ninja.resolve() if args.ninja else None
    source = args.source.resolve()
    if not cmake or not ninja or not ninja.is_file() or not (source / "CMakeLists.txt").is_file():
        parser.error("CMake, Ninja and the LLVM source tree are required")

    env = os.environ.copy()
    env["PATH"] = str(sdk / "bin") + os.pathsep + env["PATH"]
    options = OPTIONS.copy()
    options["CMAKE_MAKE_PROGRAM"] = ninja.as_posix()
    native = work / "host"
    if args.phase == "host":
        build = native
        targets = ["llvm-tblgen"]
        # LLVM 14's SmallVector relies on an indirect integer-type include
        # that newer GCC headers no longer supply. Keep the source unpatched.
        options.update(CMAKE_C_COMPILER=(sdk / "bin/gcc.exe").as_posix(),
                       CMAKE_CXX_COMPILER=(sdk / "bin/g++.exe").as_posix(), CMAKE_BUILD_TYPE="Release",
                       CMAKE_CXX_FLAGS_RELEASE="-O1 -DNDEBUG -include cstdint")
    else:
        tablegen = native / "bin/llvm-tblgen.exe"
        if not tablegen.is_file():
            parser.error("Build the host LLVM TableGen first")
        env = emscripten_environment(args.toolchain, sdk)
        build = work / "wasm"
        targets = ["LLVMBitWriter", "LLVMAnalysis", "LLVMTarget"]
        options.update(CMAKE_TOOLCHAIN_FILE=(sdk / "lib/emscripten/cmake/Modules/Platform/Emscripten.cmake").as_posix(),
                       CMAKE_BUILD_TYPE="MinSizeRel", LLVM_TABLEGEN=tablegen.as_posix(),
                       LLVM_ENABLE_THREADS="OFF", LLVM_HOST_TRIPLE="wasm32-unknown-emscripten")

    build.mkdir(parents=True, exist_ok=True)
    configure = [cmake, "-G", "Ninja", "-S", str(source), "-B", str(build)]
    configure.extend(f"-D{option}={value}" for option, value in options.items())
    run(configure, build, "configure.log", env)
    run([cmake, "--build", str(build), "--parallel", str(args.jobs), "--target", *targets], build, "build.log", env)
    print(f"LLVM_{args.phase.upper()}_READY", flush=True)


if __name__ == "__main__":
    main()

# end of fb-corpus-lbasic-llvm.py
