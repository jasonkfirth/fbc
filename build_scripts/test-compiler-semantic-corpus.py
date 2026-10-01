#!/usr/bin/env python3
"""Project: FreeBASIC compiler tests
File: test-compiler-semantic-corpus.py
Purpose: Audit semantic exports from the maintained OMA programs.
Responsibilities: Freeze inputs, compare export modes, and retain validation evidence.
This file intentionally does NOT contain: BASIC parsing or game execution checks.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import sys
import tempfile

from compiler_semantic_audit import EMISSION_SUFFIX, audit_exports, digest


# Slicks has one translation unit per subsystem. The other maintained native
# ports include their implementation modules from the listed entry point.
PROGRAMS = {
    "behold": ("Behold/Behold.bas",),
    "duel999": ("duel999/SD_Main.bas",),
    "kinematics": ("kinematics/kinematic_man_two_bodies_self_collision_friction.bas",),
    "kinematics_joint_limits": ("kinematics/kinematic_man_two_bodies_joint_limits.bas",),
    "kinematics_floor_friction": ("kinematics/kinematic_man_two_bodies_floor_friction.bas",),
    "kinematics_impulse_capsules": ("kinematics/kinematic_man_two_bodies_impulse_capsules.bas",),
    "nietzsche": ("NietzscheSE-MSDOS-1.1/Nietzsche/src/win32/win11.bas",),
    "qfak": ("QuestForAKing-Win32-1.5/src/win11.bas",),
    "rambo": ("RamboVsKittyCat-Win32-0.1/killquest.bas",),
    "starphalanx": ("StarPhalanx-win32-0.5/entryv2.bas",),
    "openmarket": ("Tamper/tamper/src/openmarket_bootstrap.bas",),
    "openslicks": tuple("Slicks n Slide/src/" + name + ".bas" for name in (
        "slicks", "slicks_app", "slicks_font", "slicks_track", "slicks_input",
        "slicks_vehicle", "slicks_game", "slicks_render", "slicks_menu")),
}


def freeze_inputs(root: Path, compiler: Path, oma: Path, output: Path) -> tuple[Path, Path]:
    inputs = output / "inputs"
    inputs.mkdir()
    shutil.copy2(compiler, inputs / "fbc")
    shutil.copytree(root / "inc", inputs / "inc")
    validation = inputs / "validation"
    validation.mkdir()
    shutil.copy2(root / "tests/semantic-sidecar/sidecar.py", validation / "sidecar.py")
    shutil.copy2(Path(__file__), validation / Path(__file__).name)
    shutil.copy2(Path(__file__).with_name("compiler_semantic_audit.py"),
                 validation / "compiler_semantic_audit.py")
    source_root = inputs / "OMA"
    for path in sorted(oma.rglob("*")):
        if path.is_file() and path.suffix.lower() in (".bas", ".bi", ".inc"):
            target = source_root / path.relative_to(oma)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    # Emission only needs the compiler and headers. Snapshot hashes include
    # every copied file so later workspace changes cannot alter this audit.
    manifest = {str(path.relative_to(inputs)): digest(path)
                for path in sorted(inputs.rglob("*")) if path.is_file()}
    (output / "inputs-sha256.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return inputs, source_root


def audit_program(inputs: Path, source_root: Path, output: Path,
                  program: str, backend: str, timeout: int) -> dict:
    work = output / backend / program
    work.mkdir(parents=True)
    modules = [source_root / path for path in PROGRAMS[program]]
    missing = [str(path) for path in modules if not path.is_file()]
    if missing:
        raise ValueError("Missing OMA module: " + ", ".join(missing))
    common = [str(inputs / "fbc"), "-prefix", str(inputs), "-i", str(inputs / "inc"),
              "-gen", backend, "-mt", "-r"]
    if backend == "gas":
        common += ["-target", "linux-x86"]
    if program == "openslicks":
        common += ["-i", str(modules[0].parent)]
    record = audit_exports(common, modules, work, backend, timeout)
    record["program"] = program
    return record


def run(options, output: Path) -> int:
    root = options.root.resolve()
    inputs, source_root = freeze_inputs(root, options.fbc.resolve(),
                                       (options.oma or root / "OMA").resolve(), output)
    sys.path.insert(0, str(inputs / "validation"))
    records = []
    for backend in options.backend or ("gcc", "llvm", "clang"):
        for program in options.program or PROGRAMS:
            try:
                record = audit_program(inputs, source_root, output, program, backend, options.timeout)
            except (ValueError, OSError) as error:
                record = {"program": program, "backend": backend, "passed": False, "error": str(error)}
            records.append(record)
            (output / "results.json").write_text(json.dumps(records, indent=2) + "\n")
            print(backend, program, "passed" if record["passed"] else record["error"], flush=True)
    failures = sum(not record["passed"] for record in records)
    print(f"{len(records) - failures}/{len(records)} semantic corpus cases passed; artifacts: {output}")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--oma", type=Path, help="OMA source tree; defaults to the project's OMA directory")
    parser.add_argument("--output", type=Path, help="Retain artifacts in a new directory")
    parser.add_argument("--backend", choices=tuple(EMISSION_SUFFIX), action="append")
    parser.add_argument("--program", choices=tuple(PROGRAMS), action="append")
    parser.add_argument("--timeout", type=int, default=180)
    options = parser.parse_args()
    if options.timeout <= 0:
        parser.error("--timeout must be positive")
    if options.output:
        output = options.output.resolve()
        output.mkdir(parents=True, exist_ok=False)
        return run(options, output)
    with tempfile.TemporaryDirectory(prefix="fbc-semantic-oma-") as temporary:
        return run(options, Path(temporary))


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-semantic-corpus.py
