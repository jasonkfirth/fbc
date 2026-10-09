"""Project: FreeBASIC semantic audits
File: compiler_semantic_audit.py
Purpose: Check semantic exports against frozen source files and generated code.
Responsibilities: Freeze reader dependencies, bound execution, validate provenance, and compare modes.
This file intentionally does NOT contain: source selection or BASIC name resolution.
"""

from __future__ import annotations

from collections import Counter
import codecs
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess


EMISSION_SUFFIX = {"gcc": ".c", "clang": ".c", "llvm": ".ll", "gas64": ".asm", "gas": ".asm"}
SEMANTIC_READER_FILES = (
    "sidecar.py",
    "semantic_flow.py",
    "semantic_literals.py",
    "semantic_queries.py",
    "semantic_selects.py",
    "semantic_select_lowering.py",
    "semantic_declarations.py",
    "semantic_procedures.py",
    "semantic_aggregate_access.py",
    "semantic_string_declarations.py",
    "semantic_repetitions.py",
    "aggregate_fields.py",
    "semantic_callbacks.py",
    "semantic_array_storage.py",
    "semantic_enums.py",
    "semantic_iif.py",
    "semantic_if.py",
    "semantic_if_arms.py",
    "semantic_assignment_inputs.py",
    "semantic_assignment_storage.py",
    "semantic_abi_policy.py",
)


def freeze_semantic_reader(root: Path, destination: Path) -> None:
    """Copy the independent reader and every module needed to import it."""
    source = root / "tests" / "semantic-sidecar"
    for name in SEMANTIC_READER_FILES:
        path = source / name
        if not path.is_file():
            raise FileNotFoundError("Semantic reader dependency is missing: " + str(path))
        shutil.copy2(path, destination / name)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(command: list[str], cwd: Path, log: Path, timeout: int) -> int:
    with log.open("w", encoding="utf-8") as stream:
        stream.write("$ " + shlex.join(command) + "\n")
        stream.flush()
        process = subprocess.Popen(command, cwd=cwd, stdin=subprocess.DEVNULL,
                                   stdout=stream, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            return process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            # Terminate the compiler's process group, including any tool it
            # launched, rather than leaving a child behind after a failed job.
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            return 124


def emission_hashes(modules: list[Path], backend: str, destination: Path,
                    emitted_paths: list[Path] | None = None) -> dict[str, str]:
    destination.mkdir()
    result = {}
    for index, module in enumerate(modules):
        path = emitted_paths[index] if emitted_paths is not None else module.with_suffix(EMISSION_SUFFIX[backend])
        if not path.is_file():
            raise ValueError("Compiler did not produce emission for " + str(module))
        name = path.name if emitted_paths is not None else module.name
        shutil.copy2(path, destination / path.name)
        result[name] = digest(path)
    return result


def physical_lines(path: Path) -> list[str]:
    data = path.read_bytes()
    for bom, encoding in ((codecs.BOM_UTF32_LE, "utf-32"), (codecs.BOM_UTF32_BE, "utf-32"),
                          (codecs.BOM_UTF16_LE, "utf-16"), (codecs.BOM_UTF16_BE, "utf-16"),
                          (codecs.BOM_UTF8, "utf-8-sig")):
        if data.startswith(bom):
            return data.decode(encoding).splitlines()
    # Unmarked legacy sources can contain single-byte comments. Preserve
    # those bytes while still counting UTF-8 text in editor UTF-16 columns.
    return data.decode("utf-8", errors="surrogateescape").splitlines()


def audit_source_ranges(model) -> dict[str, int]:
    from sidecar import source_range

    dependencies = {row[1] for row in model.records["D"]}
    if model.footer[11] != "1":
        raise ValueError("Corpus dependency list is incomplete")
    if not {row[1] for row in model.records["M"]} <= dependencies:
        raise ValueError("Committed module is missing from its dependencies")
    lines = {}
    counts = Counter()
    for tag, flag_column, range_column in (("B", 3, 4), ("E", 2, 3), ("I", 5, 6), ("O", 4, 5)):
        for row in model.records[tag]:
            if row[flag_column] != "1":
                continue
            filename, start_line, start_column, end_line, end_column = source_range(row, range_column)
            if filename not in dependencies:
                raise ValueError("Physical source fact is missing its dependency: " + filename)
            if filename not in lines:
                lines[filename] = physical_lines(Path(filename))
            source = lines[filename]
            if start_line > len(source) or end_line > len(source):
                raise ValueError("Physical range extends beyond its source: " + repr(row))
            for line, column in ((start_line, start_column), (end_line, end_column)):
                width = len(source[line - 1].encode("utf-16-le", errors="surrogatepass")) // 2
                if column > width:
                    raise ValueError("Physical range extends beyond its source line: " + repr(row))
            counts[tag] += 1
    return dict(counts)


def expression_facts(model) -> Counter:
    # Full and compact modes use separate identity domains. Compare exact
    # parser-selected source/type/operator facts with identity fields removed.
    return Counter(tuple(row[2:13] + row[15:]) for row in model.records["E"])


def audit_exports(common: list[str], modules: list[Path], work: Path,
                  backend: str, timeout: int, private_emissions: bool = False,
                  cwd: Path | None = None, keep_going: bool = False) -> dict:
    from sidecar import Model

    full = None
    baseline = None
    record = {"backend": backend, "modules": len(modules), "modes": {}}
    module_arguments = list(map(str, modules))
    emitted_paths = None
    if private_emissions:
        # Project builds can compile the same helper under different options.
        # Route emissions per invocation while keeping frozen source locations
        # and include resolution unchanged across the three export modes.
        emitted = work / "emitted"
        emitted.mkdir()
        module_arguments = []
        emitted_paths = []
        for index, module in enumerate(modules):
            object_path = emitted / (str(index) + "-" + module.stem + ".o")
            module_arguments += ["-o", str(object_path), str(module)]
            emitted_paths.append(object_path.with_suffix(EMISSION_SUFFIX[backend]))
    errors = []
    for mode in ("off", "full", "expressions"):
        model_path = work / (mode + ".tsv")
        option = [] if mode == "off" else [
            "-semantic-model" if mode == "full" else "-semantic-model-expressions", str(model_path)]
        command = [*common, *option, *module_arguments]
        record["modes"][mode] = {}
        try:
            status = invoke(command, cwd or modules[0].parent, work / (mode + ".log"), timeout)
            record["modes"][mode]["status"] = status
            if status != 0:
                raise ValueError(f"{mode} compiler exited with {status}; see {work / (mode + '.log')}")
            emitted = emission_hashes(modules, backend, work / mode, emitted_paths)
            record["modes"][mode]["emission_sha256"] = emitted
            if mode == "off":
                baseline = emitted
                record["modes"][mode]["passed"] = True
                continue
            if baseline is not None and emitted != baseline:
                raise ValueError("Semantic export changed emitted program code in " + mode + " mode")
            model = Model.read(model_path, expressions_only=mode == "expressions")
            if [row[1] for row in model.records["M"]] != [str(module) for module in modules]:
                raise ValueError("Semantic export changed or omitted a translation unit")
            if mode == "full" and any(context[5] != backend for context in model.contexts):
                raise ValueError("Semantic export reported the wrong emission backend")
            record["modes"][mode]["records"] = {tag: len(rows) for tag, rows in model.records.items()}
            record["modes"][mode]["physical_ranges"] = audit_source_ranges(model)
            if mode == "full":
                full = model
                record["modes"][mode]["symbol_classes"] = dict(Counter(row[3] for row in model.types.values()))
                record["modes"][mode]["node_kinds"] = dict(Counter(
                    model.properties["node", identity]["kind"] for identity in model.nodes))
            elif full is not None:
                if expression_facts(model) != expression_facts(full):
                    raise ValueError("Full and compact expression facts differ")
                if model.records["D"] != full.records["D"]:
                    raise ValueError("Full and compact dependency closures differ")
            record["modes"][mode]["passed"] = True
        except (ValueError, OSError) as error:
            if not keep_going:
                raise
            # A full model may exceed its documented staging budget while
            # compact export still works. Preserve both outcomes and the
            # disabled control, without accepting this whole case as passing.
            record["modes"][mode]["passed"] = False
            record["modes"][mode]["error"] = str(error)
            errors.append(str(error))
    record["passed"] = not errors
    if errors:
        record["error"] = "; ".join(errors)
    return record


# end of compiler_semantic_audit.py
