#!/usr/bin/env python3

##############################################################################
# FreeBASIC exampleageddon runner
##############################################################################
#
# Purpose:
#
#   Compile every FreeBASIC example and run the self-contained examples that are
#   safe to execute unattended.
#
# Responsibilities:
#
#   * discover examples/**/*.bas recursively
#   * classify examples that need external libraries, another OS, helper-module
#     build rules, graphics/audio/input, or network services
#   * compile examples into out/exampleageddon without writing build products
#     into examples/
#   * run self-contained examples with a timeout
#   * write CSV and Markdown reports for later comparison
#
# This file intentionally does NOT contain:
#
#   * package validation
#   * GUI automation
#   * online dependency installation
#   * correctness checks for examples that need user input or third-party APIs
#
##############################################################################

from __future__ import annotations

import argparse
import csv
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path


EXTERNAL_PATH_MARKERS = (
    "/optimizepureabstracttypes/",
    "/compression/",
    "/console/caca/",
    "/console/curses/",
    "/database/",
    "/files/freeimage/",
    "/files/gd/",
    "/files/jpeglib/",
    "/files/libpng/",
    "/graphics/allegro/",
    "/graphics/allegro5/",
    "/graphics/cairo/",
    "/graphics/freetype/",
    "/graphics/opengl/",
    "/graphics/sdl/",
    "/graphics/tinyptc/",
    "/graphics/grx/",
    "/gui/",
    "/math/cryptlib/",
    "/math/gsl/",
    "/math/ode/",
    "/manual/libraries/",
    "/network/curl/",
    "/other-languages/",
    "/regex/pcre/",
    "/regex/tre/",
    "/sound/",
    "/xml/",
    "/misc/cunit/",
    "/misc/glib/",
)

EXTERNAL_TEXT_MARKERS = (
    "allegro",
    "bass.bi",
    "bassmod",
    "bzlib",
    "cairo",
    "caca.bi",
    "cryptlib",
    "curl",
    "expat",
    "ffi.bi",
    "fmod",
    "freeimage",
    "freetype",
    "gd.bi",
    "gl/gl.bi",
    "glext",
    "glfw",
    "glu.bi",
    "gsl",
    "gtk",
    "jpeglib",
    "libxml",
    "llvm-c.bi",
    "mysql",
    "newton.bi",
    "ode/",
    "openal",
    "pcre",
    "png.bi",
    "portaudio",
    "postgres",
    "sdl/",
    "sqlite",
    "tinyptc",
    "zlib",
)

REMOTE_RETRY_ATTEMPTS = 3
REMOTE_RETRY_DELAY = 2
REMOTE_RETRY_MARKERS = (
    "banner exchange",
    "connection closed",
    "connection refused",
    "connection reset",
    "connection timed out",
    "kex_exchange_identification",
    "operation timed out",
)

PLATFORM_PATH_MARKERS = (
    "/dos/",
    "/win32/",
    "/manual/hardware/",
    "/network/win32/",
)

HELPER_MODULES = {
    "examples/dll/mydll.bas",
    "examples/dll/dylib.bas",
    "examples/dll/test.bas",
    "examples/manual/module/common1.bas",
    "examples/manual/module/common2.bas",
    "examples/manual/module/extern2.bas",
    "examples/manual/module/extern1.bas",
    "examples/manual/procs/lib.bas",
    "examples/manual/procs/mydll.bas",
    "examples/manual/proguide/shared-lib/load.bas",
    "examples/manual/proguide/shared-lib/mytest.bas",
    "examples/manual/proguide/static-lib/mytest.bas",
    "examples/manual/proguide/varscope/module1.bas",
    "examples/manual/proguide/varscope/module2.bas",
    "examples/manual/proguide/varscope/module3.bas",
    "examples/misc/trycatch/trycatch.bas",
    "examples/network/curl/CHttp/CHttp.bas",
    "examples/network/curl/CHttp/CHttpForm.bas",
    "examples/network/curl/CHttp/CHttpStream.bas",
    "examples/threads/timer-lib/timer.bas",
}

MULTIFILE_PROGRAMS = {
    "examples/misc/trycatch/test.bas": (
        "examples/misc/trycatch/trycatch.bas",
    ),
}

INTENTIONAL_FAILURES = {
    "examples/manual/control/iif.bas",
    "examples/manual/control/iif2.bas",
    "examples/manual/error/erfn.bas",
    "examples/manual/error/erl.bas",
    "examples/manual/error/ermn.bas",
    "examples/manual/error/err1.bas",
    "examples/manual/error/error.bas",
    "examples/manual/module/import.bas",
    "examples/manual/module/opts.bas",
    "examples/manual/prepro/line.bas",
    "examples/manual/prepro/inclib.bas",
    "examples/manual/prepro/error.bas",
    "examples/manual/prepro/pragma_reserve1.bas",
    "examples/manual/prepro/pragma_reserve2.bas",
    "examples/manual/prepro/pragma_reserve3.bas",
    "examples/manual/proguide/linecontinuation2.bas",
    "examples/win32/ddk/driver/driver.bas",
}

PLATFORM_SOURCES = {
    "examples/compiler/builtin-memory.bas",
    "examples/compiler/builtin-numeric.bas",
    "examples/manual/defines/fbasm.bas",
    "examples/manual/defines/fboptionprofile.bas",
    "examples/manual/defines/fbprofile.bas",
    "examples/manual/misc/asm.bas",
    "examples/manual/procs/naked1.bas",
    "examples/manual/procs/naked2.bas",
    "examples/manual/proguide/binaries/call-tree-fb-profiling.bas",
    "examples/manual/proguide/binaries/control-code-fb-profiling.bas",
    "examples/manual/proguide/binaries/directly-call-fb-profiling.bas",
    "examples/manual/proguide/binaries/report-name-fb-profiling.bas",
    "examples/manual/proguide/binaries/simple-fb-profiling-cycles.bas",
    "examples/manual/proguide/binaries/simple-fb-profiling.bas",
    "examples/manual/proguide/libs/libs3.bas",
    "examples/manual/proguide/libs/libs4.bas",
    "examples/manual/proguide/variadic_arguments/va_.bas",
    "examples/manual/proguide/variadic_arguments/va_2.bas",
    "examples/manual/proguide/variadic_arguments/va_3.bas",
    # RUN replaces the current process with a named executable. This manual
    # sample deliberately names program.exe, so both its path and failure
    # behaviour depend on the host operating system's executable conventions.
    "examples/manual/system/run.bas",
    # Recovering from SIGSEGV through signal()/longjmp() is platform behavior.
    "examples/misc/trycatch/test.bas",
}

NONTERMINATING_SOURCES = {
    "examples/manual/proguide/labels/labels_1.bas",
}

LONG_RUNNING_BENCHMARKS = {
    "examples/manual/proguide/multithreading/criticalsectionfaq15.bas",
    "examples/manual/proguide/multithreading/emulatetp4.bas",
    "examples/manual/proguide/multithreading/emulatetp5.bas",
}

INTERACTIVE_SOURCES = {
    # These samples are stored in various encodings and call MessageBox() on Windows.
    "examples/unicode/hello_chinese.bas",
    "examples/unicode/hello_greek.bas",
    "examples/unicode/hello_japanese.bas",
    "examples/unicode/hello_korean.bas",
    "examples/unicode/hello_russian.bas",
    "examples/unicode/hello_UNC.bas",
    "examples/unicode/hello_UTF16BE.bas",
    "examples/unicode/hello_UTF16LE.bas",
    "examples/unicode/hello_UTF32BE.bas",
    "examples/unicode/hello_UTF32LE.bas",
    "examples/unicode/hello_UTF8.bas",
}

INTERACTIVE_RE = re.compile(
    r"\b("
    r"getkey|inkey|input|line\s+input|messagebox|open\s+lpt|sleep|screen|screenres|"
    r"multikey|getmouse|setmouse|open\s+cons|open\s+tcp\s+server"
    r")\b",
    re.IGNORECASE,
)

NETWORK_RE = re.compile(r"\b(open\s+tcp|http-get|http_get|ftp|gethostbyname)\b", re.IGNORECASE)
THREADCALL_RE = re.compile(r"\bthreadcall\b", re.IGNORECASE)


@dataclass(frozen=True)
class Classification:
    group: str
    reason: str
    runnable: bool


@dataclass(frozen=True)
class Result:
    path: str
    group: str
    reason: str
    compile_status: str
    run_status: str
    compile_seconds: str
    run_seconds: str
    output: str
    compile_log: str
    run_log: str


def relpath(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def safe_name(path: str) -> str:
    name = path.replace("/", "__")
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)


def host_uses_windows_executables() -> bool:
    if os.name == "nt":
        return True

    system = platform.system().lower()
    return system.startswith(("msys", "mingw", "cygwin"))


def executable_name(stem: str) -> str:
    if not host_uses_windows_executables():
        return stem

    name = stem

    # Windows UAC installer detection:
    # 32-bit PE files without an application manifest can be treated as
    # installers if their file name contains words such as "setup", "install",
    # "update", or "patch".  Exampleageddon is not testing UAC, so avoid those
    # trigger words in the generated executable name.
    for word, replacement in (
        ("install", "inst"),
        ("setup", "stp"),
        ("update", "upd"),
        ("patch", "ptch"),
    ):
        name = re.sub(word, replacement, name, flags=re.IGNORECASE)

    return name + ".exe"


def fbc_package_path_entries(args: argparse.Namespace) -> list[str]:
    if not args.fbc:
        return []

    fbc_path = Path(args.fbc[0])
    entries = []

    if fbc_path.is_absolute():
        entries.append(str(fbc_path.parent))

    if host_uses_windows_executables() and args.prefix is not None:
        compiler_name = fbc_path.name.lower()
        arch_dir = ""

        if compiler_name == "fbc32.exe":
            arch_dir = "win32"
        elif compiler_name == "fbc64.exe":
            arch_dir = "win64"
        elif compiler_name == "fbcarm64.exe":
            arch_dir = "win32-aarch64"

        if arch_dir:
            entries.append(str(args.prefix / "bin" / arch_dir))

        entries.append(str(args.prefix))

    result = []
    seen = set()
    for entry in entries:
        if not entry:
            continue
        if entry in seen:
            continue
        seen.add(entry)
        result.append(entry)

    return result


def command_environment(args: argparse.Namespace, env: dict[str, str] | None = None) -> dict[str, str]:
    result = dict(env if env is not None else os.environ)
    path_entries = fbc_package_path_entries(args)

    if path_entries:
        current_path = result.get("PATH", "")
        result["PATH"] = os.pathsep.join(path_entries + ([current_path] if current_path else []))

    return result


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def classify(path: Path, root: Path, target_os: str) -> Classification:
    rel = relpath(path, root)
    lowered_path = "/" + rel.lower()
    text = read_text(path)
    lowered_text = text.lower()

    if rel in INTENTIONAL_FAILURES:
        return Classification("intentional-failure", "example intentionally demonstrates an error path", False)

    if rel in HELPER_MODULES:
        return Classification("helper-module", "source is built through a multi-file or library rule", False)

    if rel.startswith("examples/nuttx/") and target_os != "nuttx":
        return Classification("platform-specific", "example requires the NuttX runtime", False)

    if target_os == "wii" and THREADCALL_RE.search(text):
        return Classification("platform-specific", "Wii intentionally builds without libffi THREADCALL support", False)

    if rel in PLATFORM_SOURCES:
        return Classification("platform-specific", "example contains platform-specific ABI or OS assumptions", False)

    if rel in NONTERMINATING_SOURCES:
        return Classification("interactive", "example is intentionally non-terminating or control-flow oriented", False)

    if rel in LONG_RUNNING_BENCHMARKS:
        return Classification("long-running-benchmark", "benchmark intentionally exceeds the bounded smoke-test runtime", False)

    if rel in INTERACTIVE_SOURCES:
        return Classification("interactive", "example opens an interactive UI or console prompt", False)

    for marker in PLATFORM_PATH_MARKERS:
        if marker in lowered_path:
            return Classification("platform-specific", "example targets a different OS/platform", False)

    for marker in EXTERNAL_PATH_MARKERS:
        if marker in lowered_path:
            return Classification("external-library", "path belongs to a known third-party library example family", False)

    for marker in EXTERNAL_TEXT_MARKERS:
        if marker in lowered_text:
            return Classification("external-library", f"source references third-party API marker: {marker}", False)

    if NETWORK_RE.search(lowered_path) or NETWORK_RE.search(lowered_text):
        if "two-connections-selftest.bas" not in lowered_path:
            return Classification("network", "example uses network or server/client I/O", False)

    if "/graphics/" in lowered_path:
        return Classification("graphics-interactive", "graphics example needs a display/event loop", False)

    if "/sfxlib/" in lowered_path:
        return Classification("sfxlib-audio", "sfxlib example is compile-tested; unattended audio runs are skipped here", False)

    if INTERACTIVE_RE.search(text):
        return Classification("interactive", "example appears to wait for input, sleep, graphics, or console state", False)

    return Classification("self-contained", "no external, platform, graphics, audio, network, or input dependency detected", True)


def reset_directory(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def prepare_run_directory(source_dir: Path, run_dir: Path, copy_directories: bool = True) -> None:
    reset_directory(run_dir)

    for source in source_dir.iterdir():
        target = run_dir / source.name

        if source.is_dir():
            if not copy_directories:
                continue

            shutil.copytree(
                source,
                target,
                ignore=shutil.ignore_patterns(
                    "*.o",
                    "*.obj",
                    "*.asm",
                    "*.exe",
                    "gmon.out",
                    "prof-*.txt",
                ),
            )
            continue

        if source.is_file():
            if source.suffix.lower() in (".o", ".obj", ".asm", ".exe"):
                continue
            if source.name == "gmon.out" or source.name.startswith("prof-"):
                continue
            shutil.copy2(source, target)


def map_remote_path(value: str, args: argparse.Namespace) -> str:
    if not args.remote_shell:
        return value

    path = Path(value)
    if not path.is_absolute():
        return value

    resolved = path.resolve()
    for host_root, guest_root in args.path_maps:
        try:
            relative = resolved.relative_to(host_root)
        except ValueError:
            continue

        if relative.as_posix() == ".":
            return guest_root
        return guest_root.rstrip("/") + "/" + relative.as_posix()

    return value


def remote_failure_is_transient(returncode: int, output: str) -> bool:
    if returncode != 255:
        return False

    output = output.lower()
    return any(marker in output for marker in REMOTE_RETRY_MARKERS)


def remote_retry_note(attempt: int) -> str:
    return f"\nremote shell transport failed, retrying ({attempt}/{REMOTE_RETRY_ATTEMPTS})\n"


def run_remote_script(
    args: argparse.Namespace,
    script: str,
    log=None,
    timeout: int | None = None,
) -> tuple[int, str]:
    combined_output = []

    for attempt in range(1, REMOTE_RETRY_ATTEMPTS + 1):
        try:
            completed = subprocess.run(
                args.remote_shell + [script],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=timeout,
                check=False,
            )
        except subprocess.TimeoutExpired as e:
            if e.output:
                output = e.output.decode("utf-8", errors="replace")
                combined_output.append(output)
                if log is not None:
                    log.write(output)
                    log.flush()
            raise

        output = completed.stdout.decode("utf-8", errors="replace")
        combined_output.append(output)

        if log is not None:
            log.write(output)
            log.flush()

        if not remote_failure_is_transient(completed.returncode, output):
            return completed.returncode, "".join(combined_output)

        if attempt == REMOTE_RETRY_ATTEMPTS:
            return completed.returncode, "".join(combined_output)

        note = remote_retry_note(attempt)
        combined_output.append(note)
        if log is not None:
            log.write(note)
            log.flush()

        time.sleep(REMOTE_RETRY_DELAY)

    return 255, "".join(combined_output)


def sync_remote_directory(cwd: Path, args: argparse.Namespace) -> tuple[bool, str]:
    remote_cwd = map_remote_path(str(cwd), args)
    quoted_cwd = shlex.quote(remote_cwd)

    reset_status, reset_output = run_remote_script(
        args,
        f"rm -rf {quoted_cwd} && mkdir -p {quoted_cwd}",
    )
    if reset_status != 0:
        return False, reset_output

    extract_output = []
    for attempt in range(1, REMOTE_RETRY_ATTEMPTS + 1):
        tar = subprocess.Popen(
            ["tar", "-cf", "-", "."],
            cwd=str(cwd),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        extract = subprocess.run(
            args.remote_shell + [f"cd {quoted_cwd} && tar -xf -"],
            stdin=tar.stdout,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        if tar.stdout is not None:
            tar.stdout.close()
        _, tar_stderr = tar.communicate()

        if tar.returncode != 0:
            return False, tar_stderr.decode("utf-8", errors="replace")

        output = extract.stdout.decode("utf-8", errors="replace")
        extract_output.append(output)

        if extract.returncode == 0:
            return True, remote_cwd

        if not remote_failure_is_transient(extract.returncode, output):
            return False, "".join(extract_output)

        if attempt == REMOTE_RETRY_ATTEMPTS:
            return False, "".join(extract_output)

        extract_output.append(remote_retry_note(attempt))
        time.sleep(REMOTE_RETRY_DELAY)

    return True, remote_cwd


def remote_env_prefix(env: dict[str, str] | None) -> str:
    if not env:
        return ""

    parts = []
    for key in ("SFXLIB_DRIVER", "FB_GFX_DRIVER", "FBGFX"):
        if key in env:
            parts.append(f"{key}={shlex.quote(env[key])}")

    if not parts:
        return ""

    return " ".join(parts) + " "


def compile_inputs(path: Path, root: Path, compile_cwd: Path) -> list[Path]:
    rel = relpath(path, root)
    inputs = [compile_cwd / path.name]

    for extra_rel in MULTIFILE_PROGRAMS.get(rel, ()):
        extra_source = root / extra_rel
        if not extra_source.is_file():
            raise FileNotFoundError(f"multi-file example source not found: {extra_source}")

        if extra_source.parent == path.parent:
            inputs.append(compile_cwd / extra_source.name)
            continue

        target = compile_cwd / extra_source.name
        shutil.copy2(extra_source, target)
        inputs.append(target)

    return inputs


def run_command(
    cmd: list[str],
    cwd: Path,
    timeout: int,
    log_path: Path,
    args: argparse.Namespace,
    env: dict[str, str] | None = None,
) -> tuple[str, float]:
    started = time.monotonic()
    log_path.parent.mkdir(parents=True, exist_ok=True)

    with log_path.open("w", encoding="utf-8", errors="replace") as log:
        log.write("$ " + " ".join(shlex.quote(part) for part in cmd) + "\n")
        log.write("$ cwd " + str(cwd) + "\n\n")
        log.flush()

        try:
            if args.remote_shell:
                synced, remote_cwd = sync_remote_directory(cwd, args)
                if not synced:
                    log.write(remote_cwd)
                    return "fail(255)", time.monotonic() - started

                remote_cmd = [map_remote_path(part, args) for part in cmd]
                remote_bin_dir = map_remote_path(str(args.outdir / "bin"), args)
                remote_script = (
                    "mkdir -p " + shlex.quote(remote_bin_dir) + " && " +
                    "cd " + shlex.quote(remote_cwd) + " && " +
                    remote_env_prefix(env) +
                    " ".join(shlex.quote(part) for part in remote_cmd)
                )
                returncode, _ = run_remote_script(
                    args,
                    remote_script,
                    log=log,
                    timeout=timeout,
                )
            else:
                completed = subprocess.run(
                    cmd,
                    cwd=str(cwd),
                    env=command_environment(args, env),
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    timeout=timeout,
                    check=False,
                )
                returncode = completed.returncode
        except subprocess.TimeoutExpired:
            elapsed = time.monotonic() - started
            log.write(f"\nTIMEOUT after {timeout}s\n")
            return "timeout", elapsed
        except OSError as exc:
            elapsed = time.monotonic() - started
            error_code = getattr(exc, "winerror", None) or exc.errno or 1
            log.write(f"\nOSERROR {error_code}: {exc}\n")
            return f"fail({error_code})", elapsed

    elapsed = time.monotonic() - started
    if returncode == 0:
        return "pass", elapsed

    return f"fail({returncode})", elapsed


def compile_one(path: Path, root: Path, args: argparse.Namespace) -> Result:
    rel = relpath(path, root)
    classification = classify(path, root, args.target_os)
    stem = safe_name(rel[:-4])
    binary = args.outdir / "bin" / executable_name(stem)
    compile_log = args.outdir / "logs" / (stem + ".compile.log")
    run_log = args.outdir / "logs" / (stem + ".run.log")
    compile_cwd = args.outdir / "work" / stem / "compile"
    run_cwd = args.outdir / "work" / stem / "run"

    copy_directories = path.parent != root / "examples"

    prepare_run_directory(path.parent, compile_cwd, copy_directories)

    cmd = list(args.fbc)
    if args.prefix is not None:
        cmd.extend(["-prefix", str(args.prefix)])
    cmd.extend(args.fbc_arg)

    cmd.extend([
        "-i",
        str(args.include_dir),
        "-p",
        str(compile_cwd),
    ])
    if args.main_module_from_source:
        cmd.extend(["-m", path.stem])
    cmd.extend(str(source) for source in compile_inputs(path, root, compile_cwd))
    cmd.extend(["-x", str(binary)])

    compile_status, compile_elapsed = run_command(
        cmd,
        cwd=compile_cwd,
        timeout=args.compile_timeout,
        log_path=compile_log,
        args=args,
    )

    run_status = "skipped"
    run_elapsed = 0.0
    output = str(binary)

    if compile_status == "pass":
        should_run = classification.runnable
        if args.no_run:
            should_run = False
            run_status = "skipped-no-run"
        elif args.run_all and classification.group not in ("external-library", "platform-specific", "helper-module", "intentional-failure"):
            should_run = True

        if should_run:
            prepare_run_directory(path.parent, run_cwd, copy_directories)
            env = os.environ.copy()
            env.setdefault("SFXLIB_DRIVER", "null")
            env.setdefault("FB_GFX_DRIVER", "none")
            run_status, run_elapsed = run_command(
                [str(binary)],
                cwd=run_cwd,
                timeout=args.run_timeout,
                log_path=run_log,
                args=args,
                env=env,
            )
        else:
            if args.no_run:
                run_log.write_text("skipped: --no-run\n", encoding="utf-8")
            else:
                run_log.write_text(
                    f"skipped: {classification.group}: {classification.reason}\n",
                    encoding="utf-8",
                )
                run_status = "skipped-" + classification.group
    else:
        run_status = "skipped-compile-failed"
        run_log.write_text("skipped: compile did not pass\n", encoding="utf-8")

    return Result(
        path=rel,
        group=classification.group,
        reason=classification.reason,
        compile_status=compile_status,
        run_status=run_status,
        compile_seconds=f"{compile_elapsed:.3f}",
        run_seconds=f"{run_elapsed:.3f}",
        output=output if compile_status == "pass" else "",
        compile_log=str(compile_log.relative_to(args.outdir)),
        run_log=str(run_log.relative_to(args.outdir)),
    )


def write_csv(results: list[Result], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(Result.__dataclass_fields__.keys()))
        writer.writeheader()
        for result in results:
            writer.writerow(result.__dict__)


def count_by(results: list[Result], attr: str) -> dict[str, int]:
    counts: dict[str, int] = {}
    for result in results:
        key = getattr(result, attr)
        counts[key] = counts.get(key, 0) + 1
    return dict(sorted(counts.items()))


def table_rows(results: list[Result], limit: int = 80) -> list[str]:
    rows = []
    for result in results[:limit]:
        rows.append(
            f"| `{result.path}` | {result.group} | {result.compile_status} | "
            f"{result.run_status} | {result.reason} | `{result.compile_log}` |"
        )
    return rows


def write_report(results: list[Result], path: Path, args: argparse.Namespace) -> None:
    compile_failures = [result for result in results if result.compile_status != "pass"]
    run_failures = [
        result
        for result in results
        if result.compile_status == "pass" and result.run_status.startswith("fail")
    ]
    run_timeouts = [
        result
        for result in results
        if result.compile_status == "pass" and result.run_status == "timeout"
    ]
    self_contained = [result for result in results if result.group == "self-contained"]
    self_contained_problems = [
        result
        for result in self_contained
        if result.compile_status != "pass" or (not args.no_run and result.run_status != "pass")
    ]

    lines = [
        "# Exampleageddon Native Sweep",
        "",
        "This report was generated by `build_scripts/exampleageddon-freebasic.py`.",
        "",
        "## Summary",
        "",
        f"- Total `.bas` files: {len(results)}",
        f"- Compile passes: {sum(1 for result in results if result.compile_status == 'pass')}",
        f"- Compile failures/timeouts: {len(compile_failures)}",
        f"- Run passes: {sum(1 for result in results if result.run_status == 'pass')}",
        f"- Run failures: {len(run_failures)}",
        f"- Run timeouts: {len(run_timeouts)}",
        f"- Self-contained examples: {len(self_contained)}",
        f"- Self-contained problems: {len(self_contained_problems)}",
        "",
        "## Classification Counts",
        "",
    ]

    for key, count in count_by(results, "group").items():
        lines.append(f"- {key}: {count}")

    lines.extend(["", "## Compile Status Counts", ""])
    for key, count in count_by(results, "compile_status").items():
        lines.append(f"- {key}: {count}")

    lines.extend(["", "## Run Status Counts", ""])
    for key, count in count_by(results, "run_status").items():
        lines.append(f"- {key}: {count}")

    lines.extend(
        [
            "",
            "## Compile Failures And Timeouts",
            "",
            "| Example | Class | Compile | Run | Reason | Log |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
    )
    lines.extend(table_rows(compile_failures))
    if len(compile_failures) > 80:
        lines.append(f"| ... | ... | ... | ... | {len(compile_failures) - 80} more rows in `results.csv` | ... |")

    lines.extend(
        [
            "",
            "## Self-Contained Failures And Timeouts",
            "",
            "| Example | Class | Compile | Run | Reason | Log |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
    )
    if self_contained_problems:
        lines.extend(table_rows(self_contained_problems))
    else:
        if args.no_run:
            lines.append("| None | self-contained | pass | skipped-no-run | All self-contained examples compiled |  |")
        else:
            lines.append("| None | self-contained | pass | pass | All self-contained examples compiled and ran |  |")

    lines.extend(
        [
            "",
            "## Run Failures And Timeouts",
            "",
            "| Example | Class | Compile | Run | Reason | Log |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
    )
    lines.extend(table_rows(run_failures + run_timeouts))
    if len(run_failures) + len(run_timeouts) > 80:
        lines.append(f"| ... | ... | ... | ... | {len(run_failures) + len(run_timeouts) - 80} more rows in `results.csv` | ... |")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args(argv: list[str]) -> argparse.Namespace:
    root = Path(__file__).resolve().parent.parent
    cpu_count = os.cpu_count() or 1

    parser = argparse.ArgumentParser(description="Compile and run FreeBASIC examples")
    parser.add_argument("--root", type=Path, default=root)
    parser.add_argument("--outdir", type=Path, default=root / "out" / "exampleageddon")
    prefix_group = parser.add_mutually_exclusive_group()
    prefix_group.add_argument("--prefix", type=Path, default=None)
    prefix_group.add_argument(
        "--no-prefix",
        action="store_true",
        help="Let the selected compiler discover its installed runtime and toolchain",
    )
    parser.add_argument("--include-dir", type=Path, default=None)
    parser.add_argument("--fbc", default=str(root / "bin" / "fbc"))
    parser.add_argument("--fbc-arg", action="append", default=[], help="Pass an additional argument to every compiler invocation")
    parser.add_argument("--target-os", default=platform.system().lower(), help="Classify target-specific examples for this OS")
    parser.add_argument("--jobs", type=int, default=cpu_count)
    parser.add_argument("--compile-timeout", type=int, default=60)
    parser.add_argument("--run-timeout", type=int, default=15)
    parser.add_argument("--no-run", action="store_true")
    parser.add_argument("--run-all", action="store_true", help="Run compiled non-external/non-platform examples even if classified interactive")
    parser.add_argument("--fail-on-self-contained", action="store_true", help="Exit non-zero if self-contained examples fail to compile or run")
    parser.add_argument("--main-module-from-source", action="store_true", help="Pass -m <source-stem> when compiling each example")
    parser.add_argument("--remote-shell", default="", help="Run compile/run commands through this shell command, such as ssh user@host")
    parser.add_argument("--path-map", action="append", default=[], help="Map an absolute host path prefix to a guest path, as HOST=GUEST")

    args = parser.parse_args(argv)
    args.root = args.root.resolve()
    args.outdir = args.outdir.resolve()
    if args.no_prefix:
        args.prefix = None
    else:
        args.prefix = (args.prefix or args.root).resolve()
    args.include_dir = (args.include_dir or (args.root / "inc")).resolve()
    args.target_os = args.target_os.strip().lower()
    args.remote_shell = shlex.split(args.remote_shell)

    if args.remote_shell:
        args.fbc = shlex.split(args.fbc)
    else:
        fbc_path = Path(args.fbc).resolve()
        if os.name == "nt" and not fbc_path.suffix and not fbc_path.is_file():
            fbc_path = fbc_path.with_suffix(".exe")

        if fbc_path.is_file():
            args.fbc = [str(fbc_path)]
        else:
            args.fbc = shlex.split(args.fbc, posix=(os.name != "nt"))

    args.path_maps = [
        (Path(host).resolve(), guest)
        for host, guest in (item.split("=", 1) for item in args.path_map)
    ]
    args.path_maps.sort(key=lambda item: len(str(item[0])), reverse=True)

    if args.jobs < 1:
        parser.error("--jobs must be positive")

    if not args.target_os:
        parser.error("--target-os must not be empty")

    return args


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    examples_dir = args.root / "examples"

    if not examples_dir.is_dir():
        raise SystemExit(f"examples directory not found: {examples_dir}")

    if not args.fbc:
        raise SystemExit("empty FBC command")

    args.outdir.mkdir(parents=True, exist_ok=True)
    (args.outdir / "bin").mkdir(parents=True, exist_ok=True)
    (args.outdir / "logs").mkdir(parents=True, exist_ok=True)

    sources = sorted(source for source in examples_dir.rglob("*.bas") if source.is_file())
    print(f"Compiling {len(sources)} example source files with {args.jobs} job(s)")

    results: list[Result] = []
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(compile_one, source, args.root, args) for source in sources]
        for index, future in enumerate(as_completed(futures), start=1):
            result = future.result()
            results.append(result)
            print(
                f"[{index}/{len(sources)}] {result.compile_status:>8} "
                f"{result.run_status:>28} {result.path}",
                flush=True,
            )

    results.sort(key=lambda result: result.path)
    write_csv(results, args.outdir / "results.csv")
    write_report(results, args.outdir / "report.md", args)

    compile_failures = sum(1 for result in results if result.compile_status != "pass")
    run_failures = sum(
        1
        for result in results
        if result.compile_status == "pass"
        and (result.run_status.startswith("fail") or result.run_status == "timeout")
    )
    self_contained_failures = sum(
        1
        for result in results
        if result.group == "self-contained"
        and (
            result.compile_status != "pass"
            or (not args.no_run and result.run_status != "pass")
        )
    )

    print()
    print(f"Report: {args.outdir / 'report.md'}")
    print(f"CSV:    {args.outdir / 'results.csv'}")
    print(f"Compile failures/timeouts: {compile_failures}")
    print(f"Run failures/timeouts:     {run_failures}")
    print(f"Self-contained failures:   {self_contained_failures}")

    if args.fail_on_self_contained and self_contained_failures:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

##############################################################################
# end of exampleageddon-freebasic.py
##############################################################################
