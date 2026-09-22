#!/usr/bin/env python3
"""
Project: FreeBASIC compiler driver tests
----------------------------------------

File: fbc-concurrent-smoke.py

Purpose:

    Verify that independent compiler processes can consume the same sources
    without sharing compiler-owned intermediate or object files.

Responsibilities:

    * run simultaneous executable and static-library builds
    * verify every executable and archive
    * verify the historical object names exposed by -c, -C and -o
    * require the __fb_ct.inf archive member name
    * reject leaked compiler temporary files

This file intentionally does NOT contain:

    * cross-target execution
    * build-system concurrency tests
    * cleanup outside its private temporary directory
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run_command(arguments: list[str], work_directory: Path,
                timeout_seconds: int) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        arguments,
        cwd=work_directory,
        capture_output=True,
        text=True,
        timeout=timeout_seconds,
        check=False,
    )


def require_success(result: subprocess.CompletedProcess[str],
                    description: str) -> None:
    if result.returncode == 0:
        return
    raise RuntimeError(
        f"{description} failed with exit code {result.returncode}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )


def create_sources(work_directory: Path) -> None:
    padding_lines = [
        f"const probe_padding_{index} as long = {index}"
        for index in range(6000)
    ]
    (work_directory / "payload.bi").write_text(
        "\n".join([
            "'' Generated only inside this test's private directory.",
            "const probe_expected as long = 12345",
            *padding_lines,
            "",
        ]),
        encoding="utf-8",
        newline="\n",
    )
    (work_directory / "program.bas").write_text(
        "\n".join([
            "#lang \"fb\"",
            "#include once \"payload.bi\"",
            "if probe_expected <> 12345 then end 2",
            "print \"parallel-ok\"",
            "",
        ]),
        encoding="utf-8",
        newline="\n",
    )
    (work_directory / "library.bas").write_text(
        "\n".join([
            "#lang \"fb\"",
            "#include once \"payload.bi\"",
            "public function probe_value( ) as long",
            "    function = probe_expected",
            "end function",
            "",
        ]),
        encoding="utf-8",
        newline="\n",
    )


def find_temporary_paths(work_directory: Path) -> list[Path]:
    temporary_suffixes = {".o", ".obj", ".asm", ".c", ".ll", ".tmp"}
    metadata_names = {"__fb_ct.inf", "__fb_ct.inf.bas", "__fb_ct.inf.o"}
    return sorted(
        path for path in work_directory.rglob("*")
        if ".fbc-" in path.name
        or path.name in metadata_names
        or (path.is_file() and path.suffix.lower() in temporary_suffixes)
    )


def require_program_output(executable: Path, work_directory: Path,
                           timeout_seconds: int, description: str) -> None:
    result = run_command(
        [str(executable)], work_directory, timeout_seconds
    )
    require_success(result, description)
    if result.stdout.strip() != "parallel-ok":
        raise RuntimeError(
            f"{description} produced unexpected output: {result.stdout!r}"
        )


def remove_retained_intermediate(work_directory: Path,
                                 description: str) -> None:
    candidates = [
        work_directory / "program.asm",
        work_directory / "program.c",
        work_directory / "program.ll",
    ]
    retained = [path for path in candidates if path.is_file()]
    if len(retained) != 1:
        raise RuntimeError(
            f"{description} produced {len(retained)} source-derived "
            f"intermediates: {retained!r}"
        )
    retained[0].unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", required=True, type=Path)
    parser.add_argument("--ar", dest="archiver", type=Path)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--timeout", type=int, default=120)
    args = parser.parse_args()

    compiler = args.fbc.resolve(strict=True)
    if args.jobs < 2 or args.jobs > 64:
        parser.error("--jobs must be between 2 and 64")
    if args.timeout < 1:
        parser.error("--timeout must be positive")

    archiver_text = str(args.archiver) if args.archiver else shutil.which("ar")
    if not archiver_text:
        parser.error("ar was not found; pass --ar with the compiler's archiver")
    archiver = Path(archiver_text).resolve(strict=True)
    executable_suffix = ".exe" if os.name == "nt" else ""

    with tempfile.TemporaryDirectory(prefix="fbc-concurrent-smoke-") as name:
        work_directory = Path(name)
        create_sources(work_directory)

        executable_commands = [
            [
                str(compiler),
                "program.bas",
                "-x",
                f"program_{index}{executable_suffix}",
            ]
            for index in range(args.jobs)
        ]
        with ThreadPoolExecutor(max_workers=args.jobs) as executor:
            results = list(executor.map(
                lambda command: run_command(
                    command, work_directory, args.timeout
                ),
                executable_commands,
            ))

        for index, result in enumerate(results):
            require_success(result, f"parallel executable build {index}")
            executable = work_directory / f"program_{index}{executable_suffix}"
            require_program_output(
                executable,
                work_directory,
                args.timeout,
                f"parallel executable {index}",
            )

        default_object = work_directory / "program.o"
        compile_only = run_command(
            [str(compiler), "program.bas", "-c"],
            work_directory,
            args.timeout,
        )
        require_success(compile_only, "compile-only object build")
        if not default_object.is_file():
            raise RuntimeError("-c did not preserve the program.o output name")
        default_object.unlink()

        kept_executable = work_directory / f"kept{executable_suffix}"
        keep_object = run_command(
            [str(compiler), "program.bas", "-C", "-x", str(kept_executable)],
            work_directory,
            args.timeout,
        )
        require_success(keep_object, "retained-object executable build")
        if not default_object.is_file():
            raise RuntimeError("-C did not preserve the program.o output name")
        require_program_output(
            kept_executable,
            work_directory,
            args.timeout,
            "retained-object executable",
        )
        default_object.unlink()

        explicit_object = work_directory / "explicit-object.o"
        explicit_executable = work_directory / f"explicit{executable_suffix}"
        explicit_output = run_command(
            [
                str(compiler),
                "program.bas",
                "-C",
                "-o",
                str(explicit_object),
                "-x",
                str(explicit_executable),
            ],
            work_directory,
            args.timeout,
        )
        require_success(explicit_output, "explicit-object executable build")
        if not explicit_object.is_file():
            raise RuntimeError("-o did not preserve the requested object name")
        if default_object.exists():
            raise RuntimeError("-o also emitted the default program.o name")
        require_program_output(
            explicit_executable,
            work_directory,
            args.timeout,
            "explicit-object executable",
        )
        explicit_object.unlink()

        for option, description, executable_name in [
            ("-r", "backend-only output", None),
            ("-rr", "final-assembly-only output", None),
            ("-R", "retained backend output", "retained-backend"),
            ("-RR", "retained final assembly", "retained-assembly"),
        ]:
            command = [str(compiler), "program.bas", option]
            retained_executable = None
            if executable_name is not None:
                retained_executable = work_directory / (
                    executable_name + executable_suffix
                )
                command.extend(["-x", str(retained_executable)])
            retained_output = run_command(
                command, work_directory, args.timeout
            )
            require_success(retained_output, description)
            remove_retained_intermediate(work_directory, description)
            if retained_executable is not None:
                require_program_output(
                    retained_executable,
                    work_directory,
                    args.timeout,
                    description,
                )

        archive_commands = [
            [
                str(compiler),
                "library.bas",
                "-lib",
                "-x",
                f"library_{index}.a",
            ]
            for index in range(args.jobs)
        ]
        with ThreadPoolExecutor(max_workers=args.jobs) as executor:
            results = list(executor.map(
                lambda command: run_command(
                    command, work_directory, args.timeout
                ),
                archive_commands,
            ))

        for index, result in enumerate(results):
            require_success(result, f"parallel archive build {index}")
            archive = work_directory / f"library_{index}.a"
            members = run_command(
                [str(archiver), "t", str(archive)],
                work_directory,
                args.timeout,
            )
            require_success(members, f"archive listing {index}")
            member_names = members.stdout.splitlines()
            if not member_names or member_names[0] != "__fb_ct.inf":
                raise RuntimeError(
                    f"archive {index} does not begin with __fb_ct.inf: "
                    f"{member_names!r}"
                )

        kept_archive = work_directory / "library_kept.a"
        kept_object = work_directory / "library.o"
        retained_archive = run_command(
            [
                str(compiler),
                "library.bas",
                "-lib",
                "-C",
                "-x",
                str(kept_archive),
            ],
            work_directory,
            args.timeout,
        )
        require_success(retained_archive, "retained-object archive build")
        if not kept_object.is_file():
            raise RuntimeError("archive -C did not preserve library.o")
        members = run_command(
            [str(archiver), "t", str(kept_archive)],
            work_directory,
            args.timeout,
        )
        require_success(members, "retained-object archive listing")
        member_names = members.stdout.splitlines()
        if not member_names or member_names[0] != "__fb_ct.inf":
            raise RuntimeError(
                "retained-object archive does not begin with __fb_ct.inf: "
                f"{member_names!r}"
            )
        kept_object.unlink()

        leftovers = find_temporary_paths(work_directory)
        if leftovers:
            formatted = "\n".join(str(path) for path in leftovers)
            raise RuntimeError(f"compiler temporary paths remain:\n{formatted}")

    print(
        f"Concurrent fbc smoke: {args.jobs} executables and "
        f"{args.jobs} archives passed; public output names preserved"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of fbc-concurrent-smoke.py
