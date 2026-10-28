"""Project: FreeBASIC semantic sidecar
File: semantic_driver.py
Purpose: Validate driver-selected roles and ordered module input snapshots.
Responsibilities: Check bounded groups, namespace ownership, and mode omissions.
This file intentionally does NOT resolve libraries or infer driver policy.
"""

from collections import defaultdict
import re

FEATURE = "driver-module-inputs"
MAX_INPUTS = 65536
SCALARS = frozenset(("driver-module", "driver-role", "driver-source", "driver-object", "driver-main",
                     "driver-ordinal", "driver-entry", "driver-outtype", "producer-version", "producer-build-date",
                     "producer-source-sha1", "producer-fork-id"))
COUNTS = ("driver-input-count", "driver-module-library-count", "driver-module-library-path-count")
FLAGS = frozenset(("no-default-libraries", "no-runtime-startup", "static-link", "strip-symbols", "retain-object",
                   "source-emission-only", "assembly-emission-only"))
SINGLE_INPUTS = FLAGS | frozenset(("compiler-prefix", "compiler-include", "compiler-library", "sysroot", "source-options",
                                  "assembler-options", "compiler-options", "linker-options", "tool-prefix", "main-input",
                                  "subsystem", "target-prefix"))
INPUT_KINDS = SINGLE_INPUTS | frozenset(("source-input", "resource-input", "source-object", "resource-object", "object-input",
                                        "archive-input", "library-input", "library-path", "excluded-library", "predefine",
                                        "preinclude", "include-path"))


def validate_driver_inputs(model, number, subject_modules):
    groups = defaultdict(dict)
    modules = {}
    model.driver_inputs = {}
    for row in model.records["K"]:
        if not row[3].startswith(("driver-", "producer-")):
            continue
        owner = number(row[2], 1)
        namespace = model.symbols.get(owner)
        module = subject_modules.get(("symbol", owner))
        if (row[1] != "symbol" or namespace is None or namespace[3] != "8" or namespace[11] != "0"
                or module is None or model.capabilities[module].get(FEATURE) != "available"
                or row[3] in groups[owner]):
            raise ValueError("Driver input has a repeated, foreign, or unavailable namespace owner")
        if module in modules and modules[module] != owner:
            raise ValueError("Module has several driver input namespaces")
        modules[module] = owner
        key = row[3]
        if key not in SCALARS and key not in COUNTS:
            match = re.fullmatch(r"(driver-input-|driver-module-library-|driver-module-library-path-)(0|[1-9][0-9]*)", key)
            if match is None or number(match[2], 0) >= MAX_INPUTS:
                raise ValueError("Unknown or oversized driver input property")
        groups[owner][key] = row[4]
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        owner = modules.get(module)
        if owner is None:
            raise ValueError("Driver input namespace is missing")
        facts = groups[owner]
        if not (SCALARS | set(COUNTS)) <= facts.keys():
            raise ValueError("Driver input scalar or count group is incomplete")
        if number(facts["driver-module"], 1) != module:
            raise ValueError("Driver input belongs to another module")
        role = facts["driver-role"]
        if role not in ("main", "secondary", "archive-metadata", "resource"):
            raise ValueError("Unknown driver module role")
        if facts["driver-main"] not in ("0", "1") or (facts["driver-main"] == "1") != (role == "main"):
            raise ValueError("Driver role disagrees with its main flag")
        source_ordinal = number(facts["driver-ordinal"], -1)
        if role in ("resource", "archive-metadata"):
            if source_ordinal != -1:
                raise ValueError("Generated module invents a source ordinal")
        elif source_ordinal < 1:
            raise ValueError("Source module has no driver ordinal")
        number(facts["driver-outtype"], 0)
        if not facts["driver-source"] or not facts["driver-object"]:
            raise ValueError("Driver module lacks a source or object name")
        if facts["producer-version"] != model.rows[0][2]:
            raise ValueError("Producer identity disagrees with the model header")
        if re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", facts["producer-build-date"]) is None:
            raise ValueError("Invalid producer build date")
        inputs = []
        expected = set(SCALARS) | set(COUNTS)
        for prefix, count_key in (("driver-input-", COUNTS[0]), ("driver-module-library-", COUNTS[1]),
                                  ("driver-module-library-path-", COUNTS[2])):
            count = number(facts[count_key], 0)
            if count > MAX_INPUTS:
                raise ValueError("Driver input count exceeds its limit")
            for ordinal in range(count):
                key = prefix + str(ordinal)
                expected.add(key)
                if key not in facts:
                    raise ValueError("Driver input sequence is incomplete")
                if prefix == "driver-input-":
                    kind, separator, value = facts[key].partition("\t")
                    if not separator or kind not in INPUT_KINDS:
                        raise ValueError("Unknown driver input category")
                    if kind in FLAGS and value not in ("0", "1"):
                        raise ValueError("Invalid driver policy flag")
                    inputs.append((kind, value))
        if set(facts) != expected:
            raise ValueError("Driver input sequence contains uncounted entries")
        for kind in SINGLE_INPUTS:
            if sum(observed == kind for observed, _ in inputs) != 1:
                raise ValueError("Driver policy input is missing or repeated")
        if role in ("main", "secondary"):
            sources = [value for kind, value in inputs if kind == "source-input"]
            if source_ordinal > len(sources) or sources[source_ordinal - 1] != facts["driver-source"]:
                raise ValueError("Driver source ordinal disagrees with its ordered inputs")
        model.driver_inputs[module] = {"owner": owner, "properties": facts, "inputs": inputs}

# end of semantic_driver.py
