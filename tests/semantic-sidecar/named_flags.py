"""Project: FreeBASIC semantic sidecar tests
File: named_flags.py
Purpose: Prove stable flag meaning, primitive policy and normalized type storage.
Responsibilities: Native matrix, real symbols, class aliases, omissions and corruption.
This file intentionally does NOT use raw masks as the public semantic interface.
"""

import subprocess

from driver_inputs import text_rows
from semantic_flags import FEATURE, POLICY_FEATURE, GROUPS, SYMBOL_CLASSES
from semantic_normalized_types import FEATURE as TYPE_FEATURE
from sidecar import Model


def flag_names(model, identity, group):
    slot = GROUPS.index(group)
    result = set()
    for selected, fields in model.named_flag_snapshots[1]["symbol"].values():
        if selected == identity:
            result.update(filter(None, fields[slot * 2].split(",")))
    return result


def check_native_matrix(test):
    model = test.compile(test.source("dim value as long\nprint value\n"))
    fixture = test.fixture("flag-vocabulary.bas")
    executable = test.working / "flag-matrix"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(fixture), str(test.root / "src/compiler/tooling/semantic-flag-names.bas"), "-x", str(executable)]
    result = subprocess.run(command, cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stderr)
    definitions = model.flag_definitions[1]
    matrix = {}
    for line in result.stdout.splitlines():
        group, symbol_class, bit, names, unmapped = line.split("\t")
        group, symbol_class, bit, unmapped = int(group), int(symbol_class), int(bit), int(unmapped)
        raw = 0 if bit == -2 else 4294967295 if bit == -1 else 1 << bit
        expected, known = [], 0
        if 1 <= symbol_class <= len(SYMBOL_CLASSES):
            for selected_group, name, mask, classes in definitions.values():
                if selected_group == GROUPS[group] and (classes == "*" or SYMBOL_CLASSES[symbol_class - 1] in classes.split(",")):
                    mask = int(mask)
                    known |= mask
                    if raw & mask:
                        expected.append(name)
        test.assertEqual((names, unmapped), (",".join(expected), raw & ~known & 4294967295), line)
        matrix[group, symbol_class, bit] = (set(filter(None, names.split(","))), unmapped)
    test.assertEqual(len(matrix), 5 * 19 * 34)
    # Independent controls for three reused native bits. A class-inappropriate
    # interpretation must remain absent, including on invalid symbol classes.
    test.assertEqual(matrix[2, 1, 24][0], {"temporary-lifetime"})
    test.assertEqual(matrix[2, 12, 24][0], {"union-field"})
    test.assertEqual(matrix[2, 3, 24][0], {"procedure-emitted"})
    test.assertEqual(matrix[2, 4, 24], (set(), 1 << 24))
    test.assertEqual(matrix[2, 3, 19][0], {"call-cloneable"})
    test.assertEqual(matrix[2, 4, 19][0], {"argv-parameter"})
    test.assertEqual(matrix[2, 1, 19][0], {"argv-parameter"})
    test.assertEqual(matrix[2, 3, 11][0], {"exclude-mangling-parent"})
    test.assertEqual(matrix[2, 1, 11][0], {"do-not-initialize"})
    test.assertEqual(matrix[2, 2, 11], (set(), 1 << 11))
    test.assertEqual(matrix[2, 1, 15][0], {"jump-table-storage"})
    test.assertEqual(matrix[2, 3, 15], (set(), 1 << 15))
    test.assertEqual(matrix[0, 12, 13][0], {"array-descriptor"})
    test.assertEqual(matrix[0, 10, 13][0], {"array-descriptor"})
    test.assertEqual(matrix[0, 2, 13], (set(), 1 << 13))
    test.assertEqual(matrix[2, 0, -1], (set(), 4294967295))
    test.assertEqual(matrix[2, 18, -1], (set(), 4294967295))


def check_real_symbols(test):
    source = test.fixture("types.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            test.assertEqual(set(model.capabilities), {1})
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertEqual(model.capabilities[1][TYPE_FEATURE], "available")
            test.assertEqual(len(model.flag_definitions[1]), 100)
            snapshot_count = sum(len(rows) for rows in model.named_flag_snapshots[1].values())
            test.assertGreater(snapshot_count, len(model.records["T"]))
            for identity, symbol in model.symbols.items():
                state = flag_names(model, identity, "status")
                if symbol[3] == "12":
                    test.assertFalse({"temporary-lifetime", "procedure-emitted", "call-cloneable", "argv-parameter"} & state)
                elif symbol[3] == "3":
                    test.assertFalse({"union-field", "temporary-lifetime", "argv-parameter"} & state)
                elif symbol[3] == "1":
                    test.assertFalse({"union-field", "procedure-emitted", "call-cloneable"} & state)
            test.assertTrue(any("runtime" in fields[4].split(",") for _, fields in model.named_flag_snapshots[1]["symbol"].values()))
            test.assertTrue(any(row[3] == "return-method-kind" for row in model.records["K"]))
            test.assertTrue(any(row[3] == "aggregate-register-return-kind" for row in model.records["K"]))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mode in ("bindings", "expressions"):
                reduced = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(set(reduced.capabilities), {1})
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertEqual(reduced.capabilities[1][TYPE_FEATURE], "unavailable")
                test.assertFalse(reduced.named_flag_snapshots)
                test.assertFalse(reduced.flag_definitions)
                test.assertFalse(reduced.type_policies)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_type_policies(test):
    declarations = []
    for position in range(9):
        spelling = ("const " if position == 8 else "") + "long"
        for depth in range(1, 9):
            spelling += " const ptr" if position == 8 - depth else " ptr"
        declarations.append(f"dim pointer_at_{position} as {spelling} = 0\n")
    source = test.source('type FirstNominal\nvalue as long\nend type\n'
                         'type SecondNominal\nvalue as long\nend type\n'
                         'type AliasNominal as FirstNominal\n'
                         '#if defined(__FB_64BIT__) and defined(__FB_WIN32__)\n'
                         'type CLongAlias as long alias "long"\n'
                         '#else\ntype CLongAlias as integer alias "long"\n#endif\n'
                         'type CCharAlias as ubyte alias "char"\n'
                         'enum FirstEnum\nFirstElement = 1\nend enum\n'
                         'type WidthHolder\ntext as string * 7\nend type\n'
                         'dim a as FirstNominal, b as SecondNominal, fixed_text as string * 13\n'
                         'dim holder as WidthHolder, array_text(1 to 2) as string * 5\n'
                         'dim alias_value as AliasNominal, enum_value as FirstEnum\n'
                         'dim byte_text as zstring * 13, wide_text as wstring * 13\n'
                         'dim c_long_value as CLongAlias, c_char_value as CCharAlias\n'
                         'dim callback_value as function cdecl(byval item as FirstNominal ptr, ...) as long = 0\n'
                         'private sub VarargProbe cdecl(byval count as integer, ...)\n'
                         'dim arguments as cva_list = any\nend sub\n'
                         'dim pointer_value as const long ptr const ptr = 0\n'
                         'fixed_text = "known width"\nprint fixed_text, pointer_value\n'
                         'holder.text = "field"\narray_text(1) = "array"\n'
                         'print holder.text, array_text(1), @array_text(1)\n'
                         + "".join(declarations) + "print @pointer_at_0\n")
    for backend in test.backends:
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            with test.subTest(backend=backend, target=target):
                model = test.compile(source, backend=backend, extra=("-target", target))
                test.assertEqual(model.capabilities[1][POLICY_FEATURE], "available")
                policies = {fields[0]: fields for fields in model.type_policies[1].values()}
                test.assertLess(int(policies["byte"][1]), int(policies["ubyte"][1]))
                test.assertLess(int(policies["long"][1]), int(policies["integer"][1]))
                test.assertLess(int(policies["integer"][1]), int(policies["uinteger"][1]))
                test.assertEqual(policies["enum"][2], "integer")
                test.assertEqual(policies["pointer"][2], "uinteger")
                test.assertEqual(policies["integer128"][3], "signed-128")
                test.assertEqual(policies["uinteger128"][3], "unsigned-128")
                test.assertEqual(policies["wstring"][3], "unsigned-16" if target.startswith("win") else "unsigned-32")
                width = "8" if target.endswith("x86_64") else "4"
                first = model.normalized_types["symbol", test.one(model, "a", "variable"), "value"]
                second = model.normalized_types["symbol", test.one(model, "b", "variable"), "value"]
                test.assertNotEqual(first[5], second[5])
                alias = model.normalized_types["symbol", test.one(model, "alias_value", "variable"), "value"]
                test.assertEqual(alias[4:9], first[4:9])
                enumeration = model.normalized_types["symbol", test.one(model, "enum_value", "variable"), "value"]
                test.assertEqual(enumeration[4:7], ["enum", str(test.one(model, "FirstEnum", "enum")), "0"])
                c_long = model.normalized_types["symbol", test.one(model, "c_long_value", "variable"), "value"]
                test.assertEqual(c_long[4], "long" if target == "win32-x86_64" else "integer")
                test.assertEqual(c_long[10], "integer")
                c_char = model.normalized_types["symbol", test.one(model, "c_char_value", "variable"), "value"]
                test.assertEqual(c_char[4], "ubyte")
                test.assertEqual(c_char[10], "zstring")
                named_modifiers = {model.primitives[0][primitive][2]: name
                                   for primitive, name in model.mangling_modifiers[1].items()}
                test.assertEqual(named_modifiers["integer"], "c-long")
                test.assertEqual(named_modifiers["zstring"], "c-char")
                callback = model.normalized_types["symbol", test.one(model, "callback_value", "variable"), "value"]
                test.assertEqual(callback[4], "function")
                test.assertEqual(callback[6], "1")
                signature = int(callback[5])
                test.assertEqual(model.signatures[signature][2], "procedure-pointer")
                test.assertTrue(any(row[1] == str(signature) and row[4] == "vararg" for row in model.records["G"]))
                arguments = test.one(model, "arguments", "variable")
                test.assertIn(model.normalized_types["symbol", arguments, "value"][10], ("va_list", "zstring"))
                for name, primitive, unit in (("byte_text", "zstring", 1),
                                               ("wide_text", "wstring", 2 if target.startswith("win") else 4)):
                    identity = test.one(model, name, "variable")
                    storage = model.normalized_types["symbol", identity, "value"]
                    test.assertEqual(storage[4], primitive)
                    test.assertEqual(storage[9], str(unit))
                    test.assertEqual(model.types[identity][13], str(13 * unit))
                pointer = model.normalized_types["symbol", test.one(model, "pointer_value", "variable"), "value"]
                test.assertEqual(pointer[6], "2")
                # CONST before the final PTR qualifies the outer pointer;
                # CONST LONG qualifies the value after both dereferences.
                test.assertEqual(pointer[8], "101")
                test.assertEqual(pointer[9], width)
                for position in range(9):
                    identity = test.one(model, f"pointer_at_{position}", "variable")
                    selected = model.normalized_types["symbol", identity, "value"]
                    test.assertEqual(selected[4:8], ["long", "0", "8", "0"])
                    test.assertEqual(selected[8], "0" * position + "1" + "0" * (8 - position))
                    test.assertEqual(selected[9], width)
                test.assertTrue(any(row[6] == "9" and row[9] == width for row in model.records["NT"]))
                facts = [row for row in model.records["NT"] if row[4] == "fixed-string" and row[6] == "0"]
                for domain in ("expression", "node"):
                    test.assertTrue({"5", "7", "13"} <= {row[9] for row in facts if row[1] == domain})
                fixed_pointers = [row for row in model.records["NT"] if row[4] == "fixed-string" and row[6] == "1"]
                test.assertTrue(fixed_pointers)
                test.assertTrue(all(row[9] == width for row in fixed_pointers))
    qb = test.source("dim value as integer\nvalue = 1\n", "policy-qb.bas")
    model = test.compile(qb, extra=("-lang", "qb"))
    test.assertTrue(any(row[3:5] == ["integer-promotion-policy", "disabled-qb"] for row in model.records["K"]))


def check_transport_and_reset(test):
    source = test.source("dim value as long\nprint value\n")
    text = test.compile(source)
    artifact = test.working / "flags.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(artifact), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(artifact).rows, text.rows)
    second = test.source("dim other as long\nprint other\n", "second.bas")
    _, artifact = test.invoke([source, second])
    multiple = Model.read(artifact)
    test.assertEqual(set(multiple.capabilities), {1, 2})
    test.assertEqual(set(multiple.named_flag_snapshots), {1, 2})
    test.assertEqual(list(multiple.named_flag_snapshots[2]["symbol"])[0], 1)
    test.assertEqual(len(multiple.flag_definitions[2]), 100)


def check_rejection(test):
    model = test.compile(test.fixture("types.bas"))
    mutations = ("definition", "class", "mask", "definition-count", "definition-duplicate", "definition-key", "contract", "missing", "flags", "unmapped",
                 "count", "ordinal", "aggregate-owner", "procedure-owner", "return-method", "aggregate-return",
                 "policy", "policy-remap", "policy-category", "promotion", "unavailable")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def prop(key):
                return next(row for row in rows if row[0] == "K" and row[3] == key)
            if mutation in ("definition", "class", "mask"):
                row = prop("flag-definition:0")
                fields = row[4].split("\t")
                fields[{"definition": 1, "class": 3, "mask": 2}[mutation]] = {"definition": "made-up", "class": "field", "mask": "3"}[mutation]
                row[4] = "\t".join(fields)
            elif mutation == "definition-count":
                prop("flag-definition-count")[4] = "0"
            elif mutation == "definition-duplicate":
                index = rows.index(prop("flag-definition-count"))
                duplicate = prop("flag-definition:0").copy()
                duplicate[3], duplicate[4] = "flag-definition:100", "attributes\tshared\t2147483648\t*"
                rows.insert(index, duplicate)
                prop("flag-definition-count")[4] = "101"
            elif mutation == "definition-key":
                prop("flag-definition:0")[3] += ":invented"
            elif mutation == "contract":
                prop("named-flag-contract")[4] = "2"
            elif mutation in ("missing", "flags", "unmapped", "ordinal"):
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("symbol-flags:"))
                if mutation == "missing":
                    rows.remove(row)
                elif mutation == "ordinal":
                    row[3] = "symbol-flags:999999"
                else:
                    fields = row[4].split("\t")
                    fields[0 if mutation == "flags" else 1] = "not-a-native-flag" if mutation == "flags" else "4294967295"
                    row[4] = "\t".join(fields)
            elif mutation == "count":
                prop("named-flag-counts:1")[4] = "0\t0\t0"
            elif mutation in ("aggregate-owner", "procedure-owner"):
                prefix = "aggregate-flags:" if mutation == "aggregate-owner" else "procedure-flags:"
                row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
                row[2] = str(next(identity for identity, symbol in model.symbols.items() if symbol[3] == "1"))
            elif mutation in ("return-method", "aggregate-return"):
                key = "return-method-kind" if mutation == "return-method" else "aggregate-register-return-kind"
                next(row for row in rows if row[0] == "K" and row[3] == key)[4] = "invented"
            elif mutation.startswith("policy"):
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("primitive-type-policy:"))
                if mutation == "policy":
                    rows.remove(row)
                else:
                    fields = row[4].split("\t")
                    fields[2 if mutation == "policy-remap" else 3] = "invented"
                    row[4] = "\t".join(fields)
            elif mutation == "promotion":
                prop("integer-promotion-policy")[4] = "disabled-qb"
            else:
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            Model(text_rows(rows))


def check_normalized_rejection(test):
    model = test.compile(test.fixture("types.bas"))
    for mutation in ("mangling-name", "mangling-classifier", "mangling-missing",
                     "name", "nominal", "pointer-depth", "reference", "qualifiers", "width",
                     "modifier", "fixed-width", "missing-symbol", "missing-node", "missing-expression",
                     "duplicate", "unavailable"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            if mutation.startswith("mangling-"):
                row = next(row for row in rows if row[0] == "K" and row[3] == "mangling-modifier-definition:0")
                if mutation == "mangling-missing":
                    rows.remove(row)
                else:
                    fields = row[4].split("\t")
                    fields[1 if mutation == "mangling-name" else 0] = "integer" if mutation == "mangling-name" else "0"
                    row[4] = "\t".join(fields)
            elif mutation == "nominal":
                row = next(row for row in rows if row[0] == "NT" and row[5] != "0")
                row[5] = "0"
            elif mutation == "fixed-width":
                row = next(row for row in rows if row[:2] == ["NT", "symbol"] and row[4] == "fixed-string")
                row[9] = str(int(row[9]) + 1)
            elif mutation.startswith("missing-"):
                domain = mutation.split("-")[1]
                rows.remove(next(row for row in rows if row[:2] == ["NT", domain]))
            elif mutation == "duplicate":
                index = next(index for index, row in enumerate(rows) if row[:2] == ["NT", "symbol"])
                rows.insert(index, rows[index].copy())
            elif mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", TYPE_FEATURE])[3] = "unavailable"
            else:
                row = next(row for row in rows if row[0] == "NT" and row[6] == "1")
                if mutation == "name":
                    row[4] = "double" if row[4] != "double" else "long"
                elif mutation == "pointer-depth":
                    row[6], row[8] = "2", row[8] + "0"
                elif mutation == "reference":
                    row[7] = "0" if row[7] == "1" else "1"
                elif mutation == "qualifiers":
                    row[8] = ("0" if row[8][0] == "1" else "1") + row[8][1:]
                elif mutation == "width":
                    row[9] = "4" if row[9] == "8" else "8"
                else:
                    row[10] = "byte"
            Model(text_rows(rows))

    # The original schema did not advertise closure. Its otherwise valid
    # records remain readable without claiming completeness for missing NT.
    legacy = [row.copy() for row in model.rows if row[:3] != ["CAP", "1", TYPE_FEATURE]
              and not (row[0] == "K" and row[3].startswith("mangling-modifier-definition:"))]
    legacy.remove(next(row for row in legacy if row[:2] == ["NT", "expression"]))
    Model(text_rows(legacy))

# end of named_flags.py
