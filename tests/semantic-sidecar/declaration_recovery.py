"""Project: FreeBASIC declaration recovery tests
File: declaration_recovery.py
Purpose: Exercise selected recovery types and transient declaration names.
Responsibilities: Actual native fallbacks, omissions, transport and corruption.
This file intentionally does NOT accept generated names as source declarations.
"""

from driver_inputs import text_rows
from recovery_steps import recovered
from recovery_validity import encoded
from sidecar import Model

TYPE_CASES = (
    ("missing", "dim value as\n", "replacement", "parser-helper.bas"),
    ("void", "dim value as any\n", "pointer", "parser-helper.bas"),
    ("zstring", "dim value as zstring\n", "pointer", "parser-decl-symbtype.bas"),
    ("constant", "type T\nmember as long\nend type\nconst wrong as T = 1\n", "discarded", "parser-decl-const.bas"),
    ("parameter", "declare sub Broken(byval value as string * 3)\n", "pointer", "parser-decl-proc-params.bas"),
    ("result", "dim callback as function()\n", "replacement", "parser-decl-symbtype.bas"),
    ("cast", "print cast(any, 1)\n", "replacement", "parser-expr-unary.bas"),
    ("peek", "print peek(any, 0)\n", "replacement", "parser-quirk-peekpoke.bas"),
)
NAME_CASES = (
    ("variable", "dim as long = 1\n", "parser-decl-var.bas"),
    ("typedef", "type as long\n", "parser-decl-typedef.bas"),
    ("field", "type T\nas long\nend type\n", "parser-decl-struct.bas"),
    ("enum", "scope\nenum E\nstr\nend enum\nend scope\n", "parser-decl-enum.bas"),
)


def check_types(test):
    for backend in test.backends:
        for label, text, kind, native_file in TYPE_CASES:
            with test.subTest(backend=backend, type_recovery=label):
                source = test.source(text, "type-recovery-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, model = recovered(test, source, backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                test.assertTrue(any(row[0] == kind and row[3].endswith(native_file)
                                    for row in model.type_recovery.get(1, ())), (label, model.type_recovery))
                test.assertNotIn("accepted", model.expression_validity.values())


def check_symbols(test):
    for label, text, native_file in NAME_CASES:
        with test.subTest(recovery_name=label):
            source = test.source(text, "recovery-name-" + label + ".bas")
            _, model = recovered(test, source)
            rows = [row for row in model.symbol_recovery.get(1, ()) if row[0] == "generated-name"]
            test.assertTrue(any(row[11].endswith(native_file) for row in rows), model.symbol_recovery)
            test.assertTrue(all(row[1] == "0" and row[8] for row in rows))
    source = test.source("declare sub Mocked(123)\n", "recovery-mock-param.bas")
    _, model = recovered(test, source)
    rows = [row for row in model.symbol_recovery.get(1, ()) if row[0] == "mock-parameter"]
    test.assertTrue(rows)
    test.assertTrue(all(row[1:3] == ["1", "4"] for row in rows))


def check_omissions(test):
    source = test.source("declare sub Parameter(byval value as long)\nenum E\nstr\nend enum\ndim value as long = 1\nprint value\n", "no-declaration-recovery.bas")
    for mode in ("full", "expressions", "bindings"):
        model = test.compile(source, mode=mode)
        test.assertFalse(model.type_recovery)
        test.assertFalse(model.symbol_recovery)
    failed = test.source("dim value as any\n", "failed-declaration-recovery.bas")
    _, path = test.invoke([source, failed], mode="expressions", success=False)
    model = Model.read(path, expressions_only=True, allow_recovery=True)
    test.assertNotIn(1, model.type_recovery)
    test.assertTrue(model.type_recovery[2])
    restarted = test.source('#lang "fblite"\ndim value as long\n', "declaration-recovery-restart.bas")
    model = test.compile(restarted, mode="expressions")
    test.assertFalse(model.type_recovery)
    test.assertFalse(model.symbol_recovery)


def check_transport_and_corruption(test):
    source = test.source("dim value as any\n", "type-recovery-transport.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        _, text = recovered(test, source, extra=extra)
        path = test.working / ("declaration-recovery-" + abi + ".fbsem")
        test.invoke([source], mode="off", success=False,
                    extra=(*extra, "-semantic-model-expressions", str(path)))
        binary = Model.read(path, expressions_only=True, allow_recovery=True)
        test.assertEqual(text.type_recovery, binary.type_recovery)
    source = test.source("declare sub Mocked(123)\n", "declaration-recovery-corruption.bas")
    _, model = recovered(test, source)
    for mutation in ("missing", "missing-count", "kind", "class", "identity", "source", "statement", "binding"):
        with test.subTest(declaration_recovery_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[3:4] == ["native-symbol-recovery:1"])
            if mutation == "missing":
                rows.remove(row)
            elif mutation == "missing-count":
                rows = [row for row in rows if row[3:4] != ["native-symbol-recovery-count"]]
            elif mutation == "identity":
                row[3] = "native-symbol-recovery:2"
            else:
                fields = row[4].split("\t")
                index, value = {"kind": (0, "resolved-binding"), "class": (2, "1"),
                                "source": (17, "999999"), "statement": (13, "999999"),
                                "binding": (0, "generated-name")}[mutation]
                fields[index] = value
                row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(encoded(rows), expressions_only=True, allow_recovery=True)
    valid = test.source("print 1\n", "declaration-recovery-count.bas")
    model = test.compile(valid)
    rows = [row.copy() for row in model.rows if row[3:4] != ["native-type-recovery-count"]]
    with test.assertRaises(ValueError):
        Model(text_rows(rows))

# end of declaration_recovery.py
