"""Project: FreeBASIC semantic sidecar tests
File: driver_inputs.py
Purpose: Verify driver module roles, ordered inputs, and producer identities.
Responsibilities: Positive observations, deliberate omissions and corrupt groups.
This file intentionally does NOT resolve linker inputs or parse BASIC syntax.
"""

import subprocess

from semantic_driver import FEATURE
from sidecar import DETAIL_TAGS, Model


def text_rows(rows):
    rows = [row.copy() for row in rows]
    rows[-1][12] = str(sum(row[0] in DETAIL_TAGS for row in rows))
    def escape(value):
        return "".join(chr(byte) if 32 <= byte < 127 and byte != 37 else f"%{byte:02X}"
                       for byte in value.encode("utf-8", errors="surrogateescape"))
    return "\n".join("\t".join(escape(value) for value in row) for row in rows) + "\n"


def check_inputs(test):
    include_a = test.working / "search-a"
    include_b = test.working / "search-b"
    include_a.mkdir()
    include_b.mkdir()
    preinclude = test.source("const PreValue = 11\n", "preinclude.bi")
    unused_header = test.source("const NeverOpened = 1\n", "unopened.bi")
    object_file = test.working / "external.o"
    archive = test.working / "external.a"
    object_file.write_bytes(b"not consumed in emission-only mode")
    archive.write_bytes(b"not consumed in emission-only mode")
    library_path = test.working / "libraries"
    library_path.mkdir()
    source = test.source('#inclib "SourceRequested"\n#libpath "source-library-path"\n'
                         'dim value as long = PreValue + DriverValue\nprint value\n')
    extra = ("-d", "DriverValue=7", "-include", str(preinclude), "-i", str(include_a),
             "-i", str(include_b), "-i", str(include_a), "-p", str(library_path),
             "-l", "CommandRequested", "-nolib", "ExcludedRequested", str(object_file), str(archive))
    for backend in test.backends:
        with test.subTest(backend=backend):
            result, _ = test.invoke([source], backend=backend, mode="off", extra=extra)
            emitted = test.emission_path(str(source.with_suffix("")), backend)
            baseline = emitted.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(emitted.read_bytes(), baseline)
            receipt = model.driver_inputs[1]
            facts = receipt["properties"]
            inputs = receipt["inputs"]
            test.assertEqual(facts["driver-role"], "secondary")
            test.assertEqual(facts["driver-main"], "0")
            test.assertEqual(facts["driver-ordinal"], "1")
            test.assertEqual(facts["driver-source"], test.compiler_input_path(source))
            test.assertEqual(facts["producer-version"], model.rows[0][2])
            test.assertRegex(facts["producer-build-date"], r"^\d{4}-\d{2}-\d{2}$")
            for kind, value in (("predefine", "DriverValue=7"), ("preinclude", test.compiler_input_path(preinclude)),
                                ("source-input", test.compiler_input_path(source)), ("object-input", test.compiler_input_path(object_file)),
                                ("archive-input", test.compiler_input_path(archive)), ("library-input", "CommandRequested"),
                                ("excluded-library", "ExcludedRequested"), ("library-path", test.compiler_input_path(library_path))):
                test.assertIn((kind, value), inputs)
            paths = [value for kind, value in inputs if kind == "include-path"]
            wanted = [test.compiler_input_path(path) for path in (include_a, include_b, include_a)]
            test.assertTrue(any(paths[index:index + 3] == wanted for index in range(len(paths))), paths)
            count = int(facts["driver-module-library-count"])
            libraries = [facts[f"driver-module-library-{index}"] for index in range(count)]
            test.assertEqual(libraries, ["CommandRequested", "SourceRequested"])
            paths = [facts[f"driver-module-library-path-{index}"]
                     for index in range(int(facts["driver-module-library-path-count"]))]
            test.assertIn("source-library-path", paths)
            dependencies = {row[1] for row in model.records["D"]}
            test.assertIn(test.compiler_path(preinclude), dependencies)
            test.assertFalse({test.compiler_path(path) for path in (object_file, archive, unused_header)} & dependencies)
            for mode in ("bindings", "expressions"):
                compact = test.compile(source, backend=backend, mode=mode, extra=extra)
                test.assertEqual(compact.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(compact.driver_inputs)
                test.assertFalse(any(row[3].startswith(("driver-", "producer-")) for row in compact.records["K"]))
                test.assertEqual(emitted.read_bytes(), baseline)
            test.assertEqual(result.stderr, "")


def check_roles(test):
    primary = test.source("dim main_value as long = 1\nprint main_value\n", "primary.bas")
    secondary = test.source("sub Helper()\nend sub\n", "secondary.bas")
    for main_name, expected in ((None, ["secondary", "secondary"]), ("primary", ["main", "secondary"]),
                                ("secondary", ["secondary", "main"])):
        with test.subTest(main=main_name):
            extra = () if main_name is None else ("-m", main_name)
            result, artifact = test.invoke([primary, secondary], extra=extra)
            model = Model.read(artifact)
            test.assertEqual([model.driver_inputs[index]["properties"]["driver-role"] for index in (1, 2)], expected)
            test.assertEqual([model.driver_inputs[index]["properties"]["driver-ordinal"] for index in (1, 2)], ["1", "2"])
            test.assertNotEqual(model.driver_inputs[1]["owner"], model.driver_inputs[2]["owner"])
            test.assertEqual(result.stderr, "")
    restarted = test.source('#lang "fblite"\nvalue = 1\nprint value\n', "restart.bas")
    model = test.compile(restarted)
    test.assertEqual(set(model.driver_inputs), {1})
    test.assertEqual(len([row for row in model.records["K"] if row[3] == "driver-module"]), 1)
    failed = test.source("this is invalid BASIC\n", "invalid.bas")
    _, artifact = test.invoke([failed], success=False, mode="expressions")
    recovered = Model.read(artifact, expressions_only=True, allow_recovery=True)
    test.assertFalse(recovered.driver_inputs)
    test.assertEqual(recovered.capabilities[1][FEATURE], "unavailable")


def check_rejection(test):
    source = test.source("namespace Nested\ndim value as long\nend namespace\n")
    model = test.compile(source)
    owner = str(model.driver_inputs[1]["owner"])
    original = model.rows
    Model(text_rows(original))
    changes = (("driver-module", "2"), ("driver-role", "guessed"), ("driver-main", "1"),
               ("driver-ordinal", "-1"), ("driver-outtype", "-1"), ("driver-source", ""),
               ("driver-input-count", "65537"), ("driver-module-library-count", "1"),
               ("producer-version", "another compiler"), ("producer-build-date", "unknown"),
               ("driver-input-0", "guessed\tinput"))
    for key, value in changes:
        with test.subTest(key=key), test.assertRaises(ValueError):
            rows = [row.copy() for row in original]
            changed = next(row for row in rows if row[:4] == ["K", "symbol", owner, key])
            changed[4] = value
            Model(text_rows(rows))
    for key in ("driver-role", "driver-input-count", "driver-input-0", "driver-module-library-path-count"):
        with test.subTest(missing=key), test.assertRaises(ValueError):
            rows = [row.copy() for row in original if row[:4] != ["K", "symbol", owner, key]]
            Model(text_rows(rows))
    rows = [row.copy() for row in original]
    duplicate = next(row for row in rows if row[:4] == ["K", "symbol", owner, "driver-module"])
    rows.insert(-1, duplicate.copy())
    with test.assertRaises(ValueError):
        Model(text_rows(rows))
    rows = [row.copy() for row in original]
    capability = next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])
    capability[3] = "unavailable"
    with test.assertRaises(ValueError):
        Model(text_rows(rows))
    # Older producers omit the complete family, including its capability.
    old_rows = [row.copy() for row in original if not
                (row[0] == "K" and row[3].startswith(("driver-", "producer-"))
                 or row[:3] == ["CAP", "1", FEATURE])]
    Model(text_rows(old_rows))

def check_generated(test):
    source = test.source('#inclib "MetadataLibrary"\nsub LibraryEntry()\nend sub\n')
    archive = test.working / "example.a"
    _, artifact = test.invoke([source], emit=False, extra=("-lib", "-x", str(archive)))
    model = Model.read(artifact)
    test.assertEqual([receipt["properties"]["driver-role"] for receipt in model.driver_inputs.values()],
                     ["secondary", "archive-metadata"])
    generated = model.driver_inputs[2]["properties"]
    test.assertEqual(generated["driver-ordinal"], "-1")
    icon = test.source('/* XPM */\nstatic char * icon[] = {\n"1 1 1 1",\n"a c #ffffff",\n"a"\n};\n', "icon.xpm")
    # XPM is a Unix resource format. Emission needs no Linux SDK and must
    # exercise resource metadata even when this test runs on Windows.
    target = ("-target", "linux-x86_64")
    for mode in ("full", "bindings", "expressions"):
        with test.subTest(mode=mode):
            observed = test.compile(icon, mode=mode, extra=target)
            if mode == "full":
                test.assertEqual(observed.driver_inputs[1]["properties"]["driver-role"], "resource")
                test.assertIn(("resource-input", test.compiler_input_path(icon)), observed.driver_inputs[1]["inputs"])
            test.assertTrue(observed.records["B"] or observed.records["E"])
            test.assertTrue(all(row[3] == "0" for row in observed.records["B"]))
            test.assertTrue(all(row[2] == "0" for row in observed.records["E"]))
    before = icon.read_bytes()
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "inc"),
               "-r", *target, "-semantic-model", str(icon), str(icon)]
    result = subprocess.run(command, cwd=test.working, text=True, capture_output=True, timeout=60)
    test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
    test.assertEqual(icon.read_bytes(), before)
    test.assertIn("could not write semantic model", result.stdout + result.stderr)


def check_transport(test):
    source = test.source('dim value as long = 7\nprint value\n', 'driver%facts.bas')
    decoded = []
    for transport in ("binary", "text"):
        artifact = test.working / (transport + ".model")
        command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "inc"),
                   "-r", "-semantic-model", str(artifact), "-d", 'DriverText="literal%09text"']
        if transport == "text":
            command.append("-semantic-model-text")
        command.append(str(source))
        result = subprocess.run(command, cwd=test.working, capture_output=True, text=True, timeout=60)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        model = Model.read(artifact)
        test.assertIn(("predefine", 'DriverText="literal%09text"'), model.driver_inputs[1]["inputs"])
        decoded.append(model.rows)
    test.assertEqual(decoded[0], decoded[1])


def check_enum_promotions(test):
    source = test.source('enum Flags\nAlpha = 1\nBeta = 2\nend enum\n'
                         'dim value as Flags = Alpha\ndim result_value as integer\n'
                         'result_value = Alpha or Beta\nresult_value = value and Beta\n')
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            enums = model.named("Flags", "enum")
            test.assertEqual(len(enums), 1)
            enum_id = enums[0]
            expressions = {int(row[1]): row for row in model.records["E"]}
            selected = []
            for identity, operands in model.expression_operands.items():
                properties = model.properties["expression", identity]
                if "numeric-selected-left-dtype" not in properties:
                    continue
                original = [expressions[int(child)] for child in operands[4:6]]
                if not any(int(row[14]) == enum_id for row in original):
                    continue
                selected.append(identity)
                for side in ("left", "right"):
                    converted = expressions[int(properties[f"numeric-selected-{side}-expression"])]
                    test.assertEqual(converted[14], "0")
                    test.assertNotEqual(int(converted[12]) & 31, 10)
            test.assertGreaterEqual(len(selected), 2)
            original = next(expressions[int(model.expression_operands[identity][4])]
                            for identity in selected if int(expressions[int(model.expression_operands[identity][4])][14]) == enum_id)
            rows = [row.copy() for row in model.rows]
            changed = next(row for row in rows if row[:2] == ["E", original[1]])
            changed[14] = str(next(identity for identity, row in model.symbols.items() if row[3] == "3"))
            with test.assertRaisesRegex(ValueError, "Normalized type disagrees with native type identity"):
                Model(text_rows(rows))
            # Keep the native/normalized snapshots consistent so the second
            # mutation reaches the enum-specific ownership check.
            normalized = next(row for row in rows if row[:4] == ["NT", "expression", original[1], "value"])
            normalized[5] = changed[14]
            with test.assertRaisesRegex(ValueError, "nominal enum identity"):
                Model(text_rows(rows))


# end of driver_inputs.py
