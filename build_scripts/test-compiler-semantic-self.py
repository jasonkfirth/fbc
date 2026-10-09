#!/usr/bin/env python3
"""Project: FreeBASIC compiler tests
File: test-compiler-semantic-self.py
Purpose: Audit semantic exports from the compiler's own source modules.
Responsibilities: Freeze inputs, query Make's selected graph, and retain audit evidence.
This file intentionally does NOT contain: an alternate compiler build graph or linker tests.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import gzip
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

from compiler_semantic_audit import EMISSION_SUFFIX, audit_exports, digest, freeze_semantic_reader


def freeze_inputs(root: Path, compiler: Path, output: Path, backends: list[str]) -> Path:
    inputs = output / "inputs"
    inputs.mkdir()
    shutil.copy2(compiler, inputs / "fbc")
    shutil.copytree(root / "inc", inputs / "inc")
    shutil.copytree(root / "mk", inputs / "mk")
    shutil.copy2(root / "GNUmakefile", inputs / "GNUmakefile")
    # Object trees and past emissions must not enter the frozen source graph.
    source = inputs / "src/compiler"
    shutil.copytree(root / "src/compiler", source, ignore=shutil.ignore_patterns(
        "obj", "__pycache__", "*.c", "*.ll", "*.asm", "*.o", "*.exe"))
    validation = inputs / "validation"
    validation.mkdir()
    freeze_semantic_reader(root, validation)
    for name in (Path(__file__).name, "compiler_semantic_audit.py"):
        shutil.copy2(Path(__file__).with_name(name), validation / name)
    # GCC and Clang both emit .c beside the BASIC module. Private backend
    # trees prevent simultaneous cases from overwriting each other's output.
    for backend in backends:
        shutil.copytree(source, inputs / "backends" / backend / "compiler")
    manifest = {str(path.relative_to(inputs)): digest(path)
                for path in sorted(inputs.rglob("*")) if path.is_file()}
    (output / "inputs-sha256.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return inputs


def make_configuration(inputs: Path, output: Path, timeout: int, target_triplet: str | None) -> dict:
    variables = ("TARGET_TRIPLET", "TARGET_OS", "TARGET_ARCH", "FBC_SRC", "ALLFBCFLAGS",
                 "BUILD_FBC_COMPAT_DEFINES", "BUILD_FBC_TARGET_OPT", "FBC_INCLUDE_FLAGS")
    # Make's source/flag variables are whitespace-separated lists. Reject a
    # snapshot path that those lists cannot represent unambiguously.
    if any(character.isspace() or character in "$#" for character in str(inputs)):
        raise ValueError("The compiler audit output path must not contain whitespace, '$', or '#'")
    makefile = output / "configuration.mk"
    makefile.write_text("include " + str(inputs / "GNUmakefile") + "\n"
                        ".PHONY: semantic-audit-configuration\n"
                        "semantic-audit-configuration:\n" + "".join(
                            "\t$(info AUDIT_" + name + "=$(" + name + "))\n"
                            for name in variables) + "\t@:\n")
    environment = dict(os.environ)
    # A recursive make caller can carry jobserver handles and overrides. The
    # frozen graph must instead describe its own selected configuration.
    for name in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL"):
        environment.pop(name, None)
    command = ["make", "--no-print-directory", "-f", str(makefile), "semantic-audit-configuration"]
    if target_triplet is not None:
        command.append("TARGET_TRIPLET=" + target_triplet)
    result = subprocess.run(command, cwd=inputs, env=environment, stdin=subprocess.DEVNULL,
                            capture_output=True, text=True, timeout=timeout, check=False)
    (output / "make-configuration.log").write_text("$ " + shlex.join(command) + "\n"
                                                 + result.stdout + result.stderr)
    if result.returncode:
        raise ValueError("Make could not resolve the compiler source graph; see make-configuration.log")
    values = dict(line[6:].split("=", 1) for line in result.stdout.splitlines()
                  if line.startswith("AUDIT_"))
    if set(values) != set(variables):
        raise ValueError("Make configuration omitted a required compiler build variable")
    source = inputs / "src/compiler"
    units = [str(Path(name).relative_to(source)) for name in shlex.split(values["FBC_SRC"])]
    if not units or len(units) != len(set(units)):
        raise ValueError("Make selected an empty or duplicate compiler source graph")
    flags = shlex.split(values["BUILD_FBC_TARGET_OPT"]) + shlex.split(values["BUILD_FBC_COMPAT_DEFINES"])
    flags += shlex.split(values["ALLFBCFLAGS"]) + shlex.split(values["FBC_INCLUDE_FLAGS"])
    configuration = {"target_triplet": values["TARGET_TRIPLET"],
                     "target_os": values["TARGET_OS"], "target_arch": values["TARGET_ARCH"],
                     "units": units, "flags": flags}
    (output / "configuration.json").write_text(json.dumps(configuration, indent=2) + "\n")
    return configuration


def compress_artifacts(work: Path) -> None:
    # Keep logs and hashes directly readable. Compression retains the exact
    # bytes of large sidecars and emissions without exhausting the build disk.
    for path in sorted(work.rglob("*")):
        if path.is_file() and path.suffix in (".tsv", ".c", ".ll", ".asm"):
            with path.open("rb") as source, gzip.open(str(path) + ".gz", "wb", compresslevel=1) as target:
                shutil.copyfileobj(source, target)
            path.unlink()


def audit_unit(inputs: Path, output: Path, configuration: dict, unit: str,
               backend: str, timeout: int, compress: bool) -> dict:
    work = output / backend / Path(unit).with_suffix("")
    work.mkdir(parents=True)
    source = inputs / "backends" / backend / "compiler"
    module = source / unit
    common = [str(inputs / "fbc"), "-prefix", str(inputs), *configuration["flags"],
              "-i", str(source), "-gen", backend, "-r"]
    try:
        record = audit_exports(common, [module], work, backend, timeout)
    except (ValueError, OSError) as error:
        record = {"backend": backend, "passed": False, "error": str(error)}
    record["unit"] = unit
    if compress:
        compress_artifacts(work)
    return record


def run(options, output: Path) -> int:
    backends = list(dict.fromkeys(options.backend or ("gcc", "llvm", "clang")))
    inputs = freeze_inputs(options.root.resolve(), options.fbc.resolve(), output, backends)
    sys.path.insert(0, str(inputs / "validation"))
    configuration = make_configuration(inputs, output, options.timeout, options.target_triplet)
    for backend, architecture in (("gas", "x86"), ("gas64", "x86_64")):
        if backend in backends and configuration["target_arch"] != architecture:
            raise ValueError(backend + " requires a Make graph for " + architecture + "; select --target-triplet")
    units = list(dict.fromkeys(options.unit or configuration["units"]))
    unknown = set(units) - set(configuration["units"])
    if unknown:
        raise ValueError("Units are absent from Make's selected compiler graph: " + ", ".join(sorted(unknown)))
    print(f"Auditing {len(units)} Make-selected modules for "
          f"{configuration['target_os']}/{configuration['target_arch']}, "
          f"{len(backends)} backends, three export modes; compiler SHA256: {digest(inputs / 'fbc')}", flush=True)
    records = []
    # Large sidecars need CPU time to validate. Each worker owns its reader
    # state and module outputs, so separate processes can validate in parallel.
    with ProcessPoolExecutor(max_workers=options.jobs) as workers:
        futures = {workers.submit(audit_unit, inputs, output, configuration, unit, backend,
                                  options.timeout, options.compress_artifacts): (backend, unit)
                   for backend in backends for unit in units}
        for future in as_completed(futures):
            backend, unit = futures[future]
            try:
                record = future.result()
            except (ValueError, OSError) as error:
                record = {"backend": backend, "unit": unit, "passed": False, "error": str(error)}
            records.append(record)
            records.sort(key=lambda item: (item["backend"], item["unit"]))
            (output / "results.json").write_text(json.dumps(records, indent=2) + "\n")
            print(f"{len(records)}/{len(futures)} {backend} {unit}: "
                  + ("passed" if record["passed"] else record["error"]), flush=True)
    failures = sum(not record["passed"] for record in records)
    print(f"{len(records) - failures}/{len(records)} compiler semantic cases passed; artifacts: {output}")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--target-triplet", help="Select compiler host policies and target flags through Make")
    parser.add_argument("--output", type=Path, help="Retain artifacts in a new directory")
    parser.add_argument("--backend", choices=tuple(EMISSION_SUFFIX), action="append")
    parser.add_argument("--unit", action="append", help="Compiler-relative module path in the selected graph; repeatable")
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--jobs", type=int, default=2, help="Independent compiler module cases in parallel")
    parser.add_argument("--compress-artifacts", action="store_true")
    options = parser.parse_args()
    if options.timeout <= 0 or options.jobs <= 0:
        parser.error("--timeout and --jobs must be positive")
    if not options.fbc.is_file():
        parser.error("Compiler does not exist: " + str(options.fbc))
    try:
        if options.output:
            output = options.output.resolve()
            output.mkdir(parents=True, exist_ok=False)
            return run(options, output)
        with tempfile.TemporaryDirectory(prefix="fbc-semantic-self-") as temporary:
            return run(options, Path(temporary))
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print("Compiler semantic audit failed: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-semantic-self.py
