"""Project: FreeBASIC semantic sidecar tests
File: raw_abi_consumer.py
Purpose: Qualify the complete reader against native and stable-v1 raw encodings.
Responsibilities: Named type meaning, unchanged raw rows, modes and corruption.
This file intentionally does NOT recode compiler output or duplicate type lowering.
"""

from driver_inputs import text_rows
from semantic_datatypes import NATIVE_PRIMITIVE_NAMES, STABLE_PRIMITIVE_NAMES
from sidecar import Model, unescape

FIXTURES = ("types.bas", "primitive-type-inputs.bas", "scalar-for-steps.bas", "parsed-string-intrinsics.bas",
            "string-declarations.bas", "select-case-lowering.bas", "select-case-coercions.bas",
            "numeric-selected-operands.bas", "nondecimal-literal-boundaries.bas", "call-abi.bas",
            "frontend-abi.bas", "layout-requests.bas", "type_observations_probe.bas", "type_macro_roots_probe.bas")


def check_matrix(test):
    for fixture in FIXTURES:
        source = test.fixture(fixture)
        for backend in test.backends:
            normalized = None
            emission = None
            for abi, vocabulary in (("native", NATIVE_PRIMITIVE_NAMES), ("stable-v1", STABLE_PRIMITIVE_NAMES)):
                with test.subTest(fixture=fixture, backend=backend, abi=abi):
                    _, artifact = test.invoke([source], backend=backend,
                                              extra=("-semantic-type-abi", abi, "-semantic-extra-facts", "all"))
                    model = Model.read(artifact)
                    model.validate_physical_locations()
                    test.assertEqual(model.datatype_abis, {1: abi})
                    test.assertEqual(tuple(model.primitives[0][index][2] for index in range(len(vocabulary))), vocabulary)
                    raw = [[unescape(field) for field in line.split("\t")] for line in artifact.read_text().splitlines()]
                    test.assertEqual(model.rows, raw, "Reader relabeled the chosen raw encoding")
                    code = test.emission_path(source.stem, backend).read_bytes()
                    if normalized is None:
                        normalized, emission = model.normalized_types, code
                    else:
                        test.assertEqual(model.normalized_types, normalized)
                        test.assertEqual(code, emission)


def check_modes(test):
    source = test.fixture("call-abi.bas")
    for backend in test.backends:
        for target in ("linux-x86", "win32-x86_64"):
            for abi in ("native", "stable-v1"):
                for mode in ("full", "bindings", "expressions"):
                    with test.subTest(backend=backend, target=target, abi=abi, mode=mode):
                        model = test.compile(source, backend=backend, mode=mode,
                                             extra=("-target", target, "-semantic-type-abi", abi))
                        test.assertEqual(model.datatype_abis, {1: abi})
                        if mode == "full":
                            test.assertTrue(model.selected_call_abi[1]["call"])
                        else:
                            test.assertFalse(model.selected_call_abi)


def check_rejection(test):
    source = test.source('dim value as integer<128> = 1\ndim text as string\n', "raw-abi.bas")
    for abi in ("native", "stable-v1"):
        model = test.compile(source, extra=("-semantic-type-abi", abi, "-semantic-extra-facts", "all"))
        primitives = {row[2]: row[1] for row in model.primitives[0].values()}
        for mutation in ("vocabulary", "encoding-capability", "extended-capability", "numeric-target", "integer-keyword"):
            with test.subTest(abi=abi, mutation=mutation), test.assertRaises(ValueError):
                rows = [row.copy() for row in model.rows]
                if mutation == "vocabulary":
                    first = next(row for row in rows if row[0] == "Y" and row[2] == "single")
                    second = next(row for row in rows if row[0] == "Y" and row[2] == "double")
                    first[2], second[2] = second[2], first[2]
                elif mutation == "encoding-capability":
                    row = next(row for row in rows if row[:3] == ["CAP", "1", "stable-primitive-type-abi-v1"])
                    row[3] = "unavailable" if abi == "stable-v1" else "available"
                elif mutation == "extended-capability":
                    next(row for row in rows if row[:3] == ["CAP", "1", "extended-integer-type-abi"])[3] = "unavailable"
                elif mutation == "numeric-target":
                    row = next(row for row in rows if row[0] == "K" and row[3] == "assignment-target-dtype")
                    row[4] = primitives["string"]
                else:
                    row = next(row for row in rows if row[0] == "K" and row[3].startswith("primitive-type-input:")
                               and row[4].split("\t")[3] == primitives["integer128"])
                    fields = row[4].split("\t")
                    fields[3] = primitives["single"]
                    row[4] = "\t".join(fields)
                Model(text_rows(rows))


def check_legacy_reduced(test):
    source = test.source("dim value as integer<128>\n", "old-raw-abi.bas")
    for mode in ("bindings", "expressions"):
        for abi in ("native", "stable-v1"):
            with test.subTest(mode=mode, abi=abi):
                model = test.compile(source, mode=mode, extra=("-semantic-type-abi", abi))
                test.assertFalse(model.records["Q"])
                test.assertFalse(model.records["Y"])
                rows = [row.copy() for row in model.rows if not (row[0] == "CAP" and row[2] in
                        ("stable-primitive-type-abi-v1", "extended-integer-type-abi"))]
                legacy = Model(text_rows(rows), bindings_only=mode == "bindings", expressions_only=mode == "expressions")
                test.assertEqual(legacy.datatype_abis, {1: "unavailable"})
                test.assertEqual(legacy.records["S"], model.records["S"])


def check_binary(test):
    source = test.fixture("types.bas")
    for backend in test.backends:
        for abi in ("native", "stable-v1"):
            with test.subTest(backend=backend, abi=abi):
                text = test.compile(source, backend=backend, extra=("-semantic-type-abi", abi))
                artifact = test.working / ("raw-" + backend + "-" + abi + ".bin")
                test.invoke([source], mode="off", backend=backend,
                            extra=("-semantic-type-abi", abi, "-semantic-model", str(artifact),
                                   "-o", str(source.with_suffix(".o"))))
                binary = Model.read(artifact)
                test.assertEqual(binary.datatype_abis, {1: abi})
                test.assertEqual(binary.rows, text.rows)

# end of raw_abi_consumer.py
