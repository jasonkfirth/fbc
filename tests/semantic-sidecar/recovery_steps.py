"""Project: FreeBASIC semantic recovery tests
File: recovery_steps.py
Purpose: Prove native replacement and retained-prefix observations.
Responsibilities: Actual error branches, omissions, reset and corrupted receipts.
This file intentionally does NOT claim complete recovery-route coverage from a hook inventory.
"""

import os
import subprocess

from driver_inputs import text_rows
from recovery_validity import encoded
from semantic_recovery import FEATURE
from sidecar import Model


def recovered(test, source, backend="gcc", extra=()):
    result, path = test.invoke([source], mode="expressions", backend=backend, success=False, extra=extra)
    return result, Model.read(path, expressions_only=True, allow_recovery=True)


def check_steps(test):
    cases = (
        ("missing-rhs", "print 1 +\n", "retained-prefix", "parser-expr-binary.bas"),
        ("binary-type", 'print "text" * 2\n', "replacement-constant", "parser-expr-binary.bas"),
        ("unary-type", 'print -"text"\n', "replacement-constant", "parser-expr-unary.bas"),
        ("missing-cast", "print cptr(any ptr, )\n", "replacement-zero", "parser-expr-unary.bas"),
    )
    for backend in test.backends:
        for label, text, kind, native_file in cases:
            with test.subTest(backend=backend, label=label):
                source = test.source(text, "native-recovery-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, model = recovered(test, source, backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                test.assertEqual(model.capabilities[1][FEATURE], "partial")
                test.assertTrue(any(step[0] == kind and step[2].endswith(native_file)
                                    for step in model.recovery_steps.get(1, ())))
                test.assertTrue(all(step[8] == "1" for step in model.recovery_steps[1]))
                test.assertNotIn("accepted", model.expression_validity.values())


def check_binary_routes(test):
    for operator in ("+", "-", "*", "/", "\\", "mod", "shl", "shr", "and", "or", "xor", "eqv", "imp", "andalso", "orelse", "=", "<>", "<", ">", "^"):
        with test.subTest(operator=operator):
            source = test.source("print 1 " + operator + "\n", "native-recovery-route.bas")
            _, model = recovered(test, source)
            test.assertTrue(any(step[0] == "retained-prefix" for step in model.recovery_steps.get(1, ())))


def check_omissions_and_modules(test):
    source = test.source("type RecoveryBase extends object\n declare abstract sub Member()\nend type\ndim address as any ptr = procptr(RecoveryBase.Member)\nprint 0\n", "native-recovery-valid.bas")
    for mode in ("full", "expressions", "bindings"):
        with test.subTest(mode=mode):
            model = test.compile(source, mode=mode)
            test.assertFalse(model.recovery_steps)
            expected = "unavailable" if mode == "bindings" else "partial"
            test.assertEqual(model.capabilities[1][FEATURE], expected)
    failed = test.source("print 2 +\n", "native-recovery-failed.bas")
    _, path = test.invoke([source, failed], mode="expressions", success=False)
    model = Model.read(path, expressions_only=True, allow_recovery=True)
    test.assertNotIn(1, model.recovery_steps)
    test.assertTrue(model.recovery_steps[2])
    restart = test.source('#lang "fblite"\nprint 1\n', "native-recovery-restart.bas")
    model = test.compile(restart, mode="expressions")
    test.assertFalse(model.recovery_steps)
    test.assertEqual(len(model.records["M"]), 1)


def check_rejection(test):
    source = test.source("print 1 +\n", "native-recovery-corruption.bas")
    _, model = recovered(test, source)
    for mutation in ("missing-count", "missing-step", "kind", "route", "statement", "source", "configuration", "root-class", "point", "identity", "duplicate"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            step = next(row for row in rows if row[0] == "K" and row[3] == "native-expression-recovery:1")
            if mutation == "missing-count":
                rows = [row for row in rows if row[3:4] != ["native-expression-recovery-count"]]
            elif mutation == "missing-step":
                rows.remove(step)
            elif mutation == "identity":
                step[3] = "native-expression-recovery:2"
            elif mutation == "duplicate":
                rows.insert(-1, step.copy())
            else:
                fields = step[4].split("\t")
                field, value = {"kind": (0, "proved-expression"), "route": (2, "unknown-parser.bas"),
                                "statement": (4, "999999"), "source": (5, "999999"),
                                "configuration": (6, "999999"), "root-class": (9, "99"),
                                "point": (14, "0")}[mutation]
                fields[field] = value
                step[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(encoded(rows), expressions_only=True, allow_recovery=True)
    # Successful full models still require a zero-count receipt.
    valid = test.source("print 1\n", "native-recovery-count.bas")
    model = test.compile(valid)
    rows = [row.copy() for row in model.rows if row[3:4] != ["native-expression-recovery-count"]]
    with test.assertRaises(ValueError):
        Model(text_rows(rows))

def check_native_guards(test):
    executable = test.working / ("recovery-guards.exe" if test.native_windows else "recovery-guards")
    sources = [test.root / "tests/semantic-sidecar/recovery-step-guards.bas",
               test.root / "src/compiler/tooling/semantic-recovery.bas"]
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
               "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
               "-m", "recovery-step-guards", "-x", str(executable), *map(str, sources)]
    result = subprocess.run(command, cwd=test.working, text=True, capture_output=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, text=True, capture_output=True, timeout=60)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("RECOVERY_STEP_GUARDS_PASS", result.stdout)

def check_transport(test):
    source = test.source("print cast(single, )\n", "native-recovery-single.bas")
    observed = {}
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        _, text = recovered(test, source, extra=extra)
        path = test.working / ("native-recovery-" + abi + ".fbsem")
        test.invoke([source], mode="off", success=False,
                    extra=(*extra, "-semantic-model-expressions", str(path)))
        binary = Model.read(path, expressions_only=True, allow_recovery=True)
        test.assertEqual(text.recovery_steps, binary.recovery_steps)
        zeroes = [step for step in text.recovery_steps[1] if step[0] == "replacement-zero"]
        test.assertTrue(zeroes)
        observed[abi] = int(zeroes[0][10])
    test.assertEqual(observed, {"native": 17, "stable-v1": 15})

def check_source_bytes(test):
    if test.native_windows:
        test.skipTest("Raw non-UTF-8 byte filenames require a POSIX host")
    filename = os.fsdecode(b"recovery-%\t-\xff.bas")
    source = test.working / filename
    source.write_bytes(b"print 1 +\n")
    artifact = test.working / "recovery-byte-path.tsv"
    base = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-r", str(source)]
    baseline = subprocess.run(base, cwd=test.working, capture_output=True, timeout=60)
    result = subprocess.run([*base, "-semantic-model-text", "-semantic-model-expressions", str(artifact)],
                            cwd=test.working, capture_output=True, timeout=60)
    test.assertEqual((result.returncode, result.stdout, result.stderr),
                     (baseline.returncode, baseline.stdout, baseline.stderr))
    model = Model.read(artifact, expressions_only=True, allow_recovery=True)
    test.assertEqual(os.fsencode(model.recovery_steps[1][0][11]), os.fsencode(source))

# end of recovery_steps.py
