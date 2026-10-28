"""Project: FreeBASIC semantic skip tests
File: token_skips.py
Purpose: Exercise native parser skipping and its structured output.
Responsibilities: Native helper behavior, actual source paths and corrupt records.
This file intentionally does NOT infer skipped tokens by reparsing BASIC.
"""

import subprocess

from recovery_steps import recovered
from recovery_validity import encoded
from semantic_skips import FEATURE
from sidecar import Model


def check_native_helper(test):
    original = (test.root / "src/compiler/parser/parser-helper.bas").read_text()
    start = original.index("sub hSkipUntil _")
    end = original.index("\nend sub", start) + len("\nend sub")
    # Compile the current production routine with controlled lexer callbacks.
    # The expected counts below live in the fixture, independent of its body.
    source = test.working / "token-skip-guards.bas"
    source.write_text((test.root / "tests/semantic-sidecar/token-skip-guards.bas").read_text()
                      + "\n" + original[start:end] + "\n' end of token-skip-guards.bas\n")
    executable = test.working / ("token-skip-guards.exe" if test.native_windows else "token-skip-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             str(source), "-x", str(executable)], cwd=test.working,
                            capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("TOKEN_SKIP_GUARDS_PASS", result.stdout)

def check_observations(test):
    cases = (
        ("target", "print ()\n", "target"),
        ("eof", "print (1", "end-of-file"),
        ("comma", "declare sub Broken(byref value as any = Ignored(1,2), byval extra as long)\n", "comma"),
        ("boundary", "const = 1 : print 2\n", "statement-boundary"),
        ("line", "@ bogus\nprint 2\n", "end-of-line"),
    )
    for backend in test.backends:
        for label, text, reason in cases:
            with test.subTest(backend=backend, skip=label):
                source = test.source(text, "token-skip-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, model = recovered(test, source, backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                test.assertEqual(model.capabilities[1][FEATURE], "partial")
                test.assertTrue(any(step[26] == reason for step in model.skip_steps.get(1, ())), (label, model.skip_steps))


def check_rejection(test):
    source = test.source("print ()\n", "token-skip-corruption.bas")
    _, model = recovered(test, source)
    for mutation in ("missing", "count", "identity", "reason", "flag", "source", "statement", "consumed", "nested", "point"):
        with test.subTest(skip_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            step = next(row for row in rows if row[3:4] == ["native-token-skip:1"])
            if mutation == "missing":
                rows.remove(step)
            elif mutation == "count":
                next(row for row in rows if row[3:4] == ["native-token-skip-count"])[4] = "0"
            elif mutation == "identity":
                step[3] = "native-token-skip:2"
            else:
                fields = step[4].split("\t")
                index, value = {"reason": (26, "end-of-file"), "flag": (6, "2"),
                                "source": (13, "999999"), "statement": (8, "999999"),
                                "consumed": (1, "0"), "nested": (7, "1"), "point": (16, "0")}[mutation]
                fields[index] = value
                step[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(encoded(rows), expressions_only=True, allow_recovery=True)


def check_transport(test):
    source = test.source("print ()\n", "token-skip-binary.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        _, text = recovered(test, source, extra=extra)
        path = test.working / ("token-skip-" + abi + ".fbsem")
        test.invoke([source], mode="off", success=False,
                    extra=(*extra, "-semantic-model-expressions", str(path)))
        binary = Model.read(path, expressions_only=True, allow_recovery=True)
        test.assertEqual(text.skip_steps, binary.skip_steps)
    valid = test.source("print 1\n", "token-skip-empty.bas")
    for mode in ("full", "bindings", "expressions"):
        model = test.compile(valid, mode=mode)
        test.assertFalse(model.skip_steps)
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable" if mode == "bindings" else "partial")

# end of token_skips.py
