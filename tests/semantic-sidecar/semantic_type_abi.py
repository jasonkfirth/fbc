"""Project: FreeBASIC semantic sidecar tests
File: semantic_type_abi.py
Purpose: Keep optional capture requests and raw datatype encoding qualified.
Responsibilities: Independent request subsets, exact emission and actual ABI code.
This file intentionally does NOT contain: alternative parsing or numeric analysis.
"""
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess

from sidecar import Model


def check_requests(test) -> None:
    test.fixture("requested-source-facts.bi")
    source = test.fixture("requested-source-facts.bas")
    code = test.emission_path(source.stem, "gcc")
    expected_code = None
    groups = [
        ("hints", ("identifier-type-hints",), ("identifier-hint-",)),
        ("declarators", ("native-declarator-lists", "source-formal-list-closure"),
         ("native-list", "source-signature")),
        ("keywords", ("consumed-lexical-keywords",), ("consumed-keyword",)),
        ("macro-access", ("macro-source-access-roles",), ("macro-source-access",)),
        ("includes", ("counted-include-requests", "include-request-scopes"),
         ("include-request-count:", "include-request-scope:")),
        ("builtins", ("counted-builtin-function-uses",), ("builtin-function-use:",)),
        ("literal-groups", ("string-literal-adjacency-inputs",),
         ("string-literal-adjacency-input",)),
        ("preprocessor", ("counted-preprocessor-events",),
         ("preprocessor-event-counts:",)),
        ("initializers", ("array-initializer-input-closure",),
         ("original-array-initializer-count",)),
        ("type-shapes", ("written-type-shapes", "written-type-root-origins"),
         ("type-shape-", "accepted-type-shape-")),
        ("native-bytes", ("native-byte-conversion-inputs",), ("native-byte-",)),
    ]

    group_bits = {name: 1 << bit for bit, (name, _, _) in enumerate(groups)}

    def request(options: tuple[str, ...], wanted: int, mode: str = "full") -> None:
        nonlocal expected_code
        _, path = test.invoke([source], mode=mode,
                              extra=("-semantic-type-abi", "native", *options))
        model = Model.read(path, bindings_only=mode == "bindings",
                           expressions_only=mode == "expressions")
        model.validate_physical_locations()
        capabilities = model.capabilities[1]
        rows = model.records.get("K", [])
        for bit, (_, features, prefixes) in enumerate(groups):
            enabled = bool(wanted & (1 << bit)) and mode == "full"
            test.assertTrue(all(capabilities[feature] == ("available" if enabled else "unavailable")
                                for feature in features), (options, mode, features))
            count = sum(row[3].startswith(prefixes) for row in rows)
            test.assertEqual(count > 0, enabled, (options, mode, prefixes, count))
        initializer_enabled = bool(wanted & group_bits["initializers"]) and mode == "full"
        counts = [int(row[4]) for row in rows if row[3] == "original-array-initializer-count"]
        edges = [row for row in model.records.get("H", []) if row[5] == "array-initializer-target"]
        test.assertEqual(counts, [1] if initializer_enabled else [])
        test.assertEqual(len(edges), 1 if initializer_enabled else 0)
        byte_enabled = bool(wanted & group_bits["native-bytes"]) and mode == "full"
        byte_counts = [int(row[4]) for row in rows if row[3] == "native-byte-count:1"]
        byte_edges = [row for row in model.records.get("H", []) if row[5] == "native-byte-input"]
        test.assertEqual(byte_counts, [1] if byte_enabled else [])
        test.assertEqual(len(byte_edges), 1 if byte_enabled else 0)
        emitted = hashlib.sha256(code.read_bytes()).hexdigest()
        if expected_code is None:
            expected_code = emitted
        test.assertEqual(emitted, expected_code, "Capture requests changed generated C")
        path.unlink()

    request((), 0)
    all_groups = (1 << len(groups)) - 1
    # Keep complete subset coverage for the original small request surface.
    # Larger surfaces use every singleton, every pair, each all-but-one set
    # and all groups together. This covers activation and interference without
    # doubling compiler launches whenever another independent group is added.
    if len(groups) <= 8:
        masks = range(1, all_groups + 1)
    else:
        masks = {all_groups}
        for first in range(len(groups)):
            masks.add(1 << first)
            masks.add(all_groups ^ (1 << first))
            for second in range(first + 1, len(groups)):
                masks.add((1 << first) | (1 << second))
        masks = sorted(masks)
    for wanted in masks:
        names = ",".join(name for bit, (name, _, _) in enumerate(groups) if wanted & (1 << bit))
        request(("-semantic-extra-facts", names), wanted)
    request(("-semantic-extra-facts", "all"), all_groups)
    request(("-semantic-extra-facts", "all", "-semantic-extra-facts", "none"), 0)
    request(("-semantic-extra-facts", "hints", "-semantic-extra-facts", "keywords"), 5)
    request(("-semantic-extra-facts", " HINTS , Keywords "), 5)
    for mode in ("bindings", "expressions"):
        request(("-semantic-extra-facts", "all"), 0, mode)
    for ordinal, invalid in enumerate(("", "unknown", "hints,", ",hints", "hints,,keywords", "hints,none")):
        bad_source = test.source(source.read_text(encoding="utf-8"), f"invalid-request-{ordinal}.bas")
        _, path = test.invoke([bad_source], success=False,
                              extra=("-semantic-extra-facts", invalid))
        test.assertFalse(path.exists(), invalid)
        test.assertFalse(test.emission_path(bad_source.stem, "gcc").exists(), invalid)


def check_type_bits(test) -> None:
    source = test.fixture("type-abi-unit.bas")
    module = test.root / "src/compiler/tooling/semantic-type-abi.bas"
    object_path = test.working / "type-abi.o"
    executable = test.working / ("type-abi.exe" if test.native_windows else "type-abi")
    flags = {"creationflags": subprocess.CREATE_NO_WINDOW} if test.native_windows else {}
    # An existing SDK can have an older library layout than the compiler tree.
    # Let its native driver link the emitted objects, as the bootstrap build
    # does, rather than making the test copy or rearrange SDK libraries.
    names = (("fbc64.exe", "bin/fbc.exe", "fbc.exe", "bin/fbc", "fbc") if test.native_windows
             else ("bin/fbc", "fbc"))
    candidates = [test.toolchain_prefix / name for name in names]
    linker = next((path for path in candidates if path.is_file()), test.compiler)
    common = [str(test.compiler), "-prefix", test.toolchain_prefix.as_posix(), "-w", "all", "-exx",
              "-i", (test.root / "src/compiler").as_posix()]
    for stable in (False, True):
        definitions = ["-d", "FBC_SEMANTIC_STABLE_TYPE_ABI"] if stable else []
        commands = [
            [*common, *definitions, "-c", str(module), "-o", str(object_path)],
            [*common, "-m", source.stem, "-c", str(source), "-o", str(test.working / "unit.o")],
            [str(linker), "-prefix", test.toolchain_prefix.as_posix(), str(test.working / "unit.o"),
             str(object_path), "-x", str(executable)],
            [str(executable), "-1" if stable else "0"],
        ]
        for command in commands:
            result = subprocess.run(command, cwd=test.working, capture_output=True, text=True,
                                    timeout=120, check=False, **flags)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertFalse(result.stderr)
        test.assertEqual(result.stdout.strip(), "TYPE_ABI_UNIT_PASS checks= 73728")

def check_hint_owners(test) -> None:
    source = test.fixture("hint-binding-ownership.bas")
    code = test.emission_path(source.stem, "gcc")
    expected_code = None
    for request in ("none", "hints", "all"):
        _, path = test.invoke([source], extra=("-semantic-type-abi", "native",
                                              "-semantic-extra-facts", request))
        model = Model.read(path)
        model.validate_physical_locations()
        emitted = hashlib.sha256(code.read_bytes()).hexdigest()
        if expected_code is None:
            expected_code = emitted
        test.assertEqual(emitted, expected_code, "Hint ownership changed generated C")
        # An unnamed formal inside a function-pointer type has a symbol for
        # ABI checking, but no physical identifier token and no B occurrence.
        # Source policies must not receive an unowned hint for that symbol.
        synthetic = {row[1] for row in model.records.get("T", [])
                     if row[3] == "parameter" and not row[2]}
        test.assertGreaterEqual(len(synthetic), 4)
        inputs = {row[3].split(":", 1)[1]: row for row in model.records.get("K", [])
                  if row[3].startswith("identifier-type-hint:")}
        if request == "none":
            test.assertFalse(inputs)
        else:
            test.assertTrue(inputs)
            test.assertFalse(synthetic & {row[2] for row in inputs.values()})
            links = {row[4] for row in model.records.get("K", [])
                     if row[3].startswith("identifier-hint-binding:")}
            test.assertEqual(set(inputs), links, "Hint input has no independent source binding")
            test.assertEqual(len(links), len(model.records.get("B", [])))
        path.unlink()


# end of semantic_type_abi.py
