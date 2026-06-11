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
    # Recovering from SIGSEGV through signal()/longjmp() is platform behavior.
    "examples/misc/trycatch/test.bas",
}

NONTERMINATING_SOURCES = {
    "examples/manual/proguide/labels/labels_1.bas",
}

INTERACTIVE_RE = re.compile(
    r"\b("
    r"getkey|inkey|input|line\s+input|sleep|screen|screenres|"
    r"multikey|getmouse|setmouse|open\s+cons|open\s+tcp\s+server"
    r")\b",
    re.IGNORECASE,
)

NETWORK_RE = re.compile(r"\b(open\s+tcp|http-get|http_get|ftp|gethostbyname)\b", re.IGNORECASE)


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


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def classify(path: Path, root: Path) -> Classification:
    rel = relpath(path, root)
    lowered_path = "/" + rel.lower()
    text = read_text(path)
    lowered_text = text.lower()

    if rel in INTENTIONAL_FAILURES:
        return Classification("intentional-failure", "example intentionally demonstrates an error path", False)

    if rel in HELPER_MODULES:
        return Classification("helper-module", "source is built through a multi-file or library rule", False)

    if rel in PLATFORM_SOURCES:
        return Classification("platform-specific", "example contains platform-specific ABI or OS assumptions", False)

    if rel in NONTERMINATING_SOURCES:
        return Classification("interactive", "example is intentionally non-terminating or control-flow oriented", False)

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


def sync_remote_directory(cwd: Path, args: argparse.Namespace) -> tuple[bool, str]:
    remote_cwd = map_remote_path(str(cwd), args)
    quoted_cwd = shlex.quote(remote_cwd)

    reset = args.remote_shell + [f"rm -rf {quoted_cwd} && mkdir -p {quoted_cwd}"]
    reset_done = subprocess.run(reset, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    if reset_done.returncode != 0:
        return False, reset_done.stdout.decode("utf-8", errors="replace")

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
    if extract.returncode != 0:
        return False, extract.stdout.decode("utf-8", errors="replace")

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
                completed = subprocess.run(
                    args.remote_shell + [remote_script],
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    timeout=timeout,
                    check=False,
                )
            else:
                completed = subprocess.run(
                    cmd,
                    cwd=str(cwd),
                    env=env,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    timeout=timeout,
                    check=False,
                )
        except subprocess.TimeoutExpired:
            elapsed = time.monotonic() - started
            log.write(f"\nTIMEOUT after {timeout}s\n")
            return "timeout", elapsed

    elapsed = time.monotonic() - started
    if completed.returncode == 0:
        return "pass", elapsed

    return f"fail({completed.returncode})", elapsed


def compile_one(path: Path, root: Path, args: argparse.Namespace) -> Result:
    rel = relpath(path, root)
    classification = classify(path, root)
    stem = safe_name(rel[:-4])
    binary = args.outdir / "bin" / stem
    compile_log = args.outdir / "logs" / (stem + ".compile.log")
    run_log = args.outdir / "logs" / (stem + ".run.log")
    compile_cwd = args.outdir / "work" / stem / "compile"
    run_cwd = args.outdir / "work" / stem / "run"

    copy_directories = path.parent != root / "examples"

    prepare_run_directory(path.parent, compile_cwd, copy_directories)

    cmd = args.fbc + [
        "-prefix",
        str(args.prefix),
        "-i",
        str(args.include_dir),
        "-p",
        str(compile_cwd),
    ]
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


def write_report(results: list[Result], path: Path) -> None:
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
        if result.compile_status != "pass" or result.run_status != "pass"
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
    parser.add_argument("--prefix", type=Path, default=None)
    parser.add_argument("--include-dir", type=Path, default=None)
    parser.add_argument("--fbc", default=str(root / "bin" / "fbc"))
    parser.add_argument("--jobs", type=int, default=cpu_count)
    parser.add_argument("--compile-timeout", type=int, default=60)
    parser.add_argument("--run-timeout", type=int, default=15)
    parser.add_argument("--no-run", action="store_true")
    parser.add_argument("--run-all", action="store_true", help="Run compiled non-external/non-platform examples even if classified interactive")
    parser.add_argument("--fail-on-self-contained", action="store_true", help="Exit non-zero if self-contained examples fail to compile or run")
    parser.add_argument("--remote-shell", default="", help="Run compile/run commands through this shell command, such as ssh user@host")
    parser.add_argument("--path-map", action="append", default=[], help="Map an absolute host path prefix to a guest path, as HOST=GUEST")

    args = parser.parse_args(argv)
    args.root = args.root.resolve()
    args.outdir = args.outdir.resolve()
    args.prefix = (args.prefix or args.root).resolve()
    args.include_dir = (args.include_dir or (args.root / "inc")).resolve()
    args.fbc = shlex.split(args.fbc)
    args.remote_shell = shlex.split(args.remote_shell)
    args.path_maps = [
        (Path(host).resolve(), guest)
        for host, guest in (item.split("=", 1) for item in args.path_map)
    ]
    args.path_maps.sort(key=lambda item: len(str(item[0])), reverse=True)

    if args.jobs < 1:
        parser.error("--jobs must be positive")

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
    write_report(results, args.outdir / "report.md")

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
        and (result.compile_status != "pass" or result.run_status != "pass")
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
