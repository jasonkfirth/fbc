"""Project: FreeBASIC semantic recovery tests
File: recovery_validity.py
Purpose: Distinguish accepted, provisional and recovery-affected expressions.
Responsibilities: Native attempt boundaries and explicit recovery marker integrity.
This file intentionally does NOT certify valid subexpressions in a rejected statement.
"""

from collections import Counter

from sidecar import DETAIL_TAGS, Model


def encoded(rows):
    def escape(value):
        return "".join(chr(byte) if 32 <= byte < 127 and byte != 37 else f"%{byte:02X}"
                       for byte in value.encode("utf-8", errors="surrogateescape"))
    return "\n".join("\t".join(escape(field) for field in row) for row in rows) + "\n"


def check_regions(test):
    source = test.source("dim good as long = 2\nprint 1 +\nprint good * 3\n", "recovery-middle.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            _, artifact = test.invoke([source], mode="expressions", backend=backend, success=False)
            model = Model.read(artifact, expressions_only=True, allow_recovery=True)
            test.assertEqual(model.recovered_modules, {1})
            test.assertEqual(model.module_validity, {1: "recovery"})
            before, affected, after = set(), set(), set()
            for row in model.records["E"]:
                identity = int(row[1])
                line = int(row[4])
                status = model.expression_validity[identity]
                if line == 2:
                    affected.add(identity)
                    test.assertEqual(status, "recovery-affected")
                else:
                    (before if line == 1 else after).add(identity)
                    test.assertEqual(status, "provisional")
            test.assertTrue(before and affected and after)
            test.assertNotIn("accepted", model.expression_validity.values())
    valid = test.source("dim good as long = 2\nprint good * 3\n", "recovery-valid.bas")
    model = test.compile(valid, mode="expressions")
    test.assertEqual(model.module_validity, {1: "accepted"})
    test.assertEqual(set(model.expression_validity.values()), {"accepted"})
    for label, text in (
            ("default", 'declare sub BrokenDefault(byval value as long = "bad")\nprint 1\n'),
            ("include", 'print 1\n#include "missing-validity.bi"\nprint 2\n')):
        with test.subTest(label=label):
            source = test.source(text, "recovery-" + label + ".bas")
            _, artifact = test.invoke([source], mode="expressions", success=False)
            model = Model.read(artifact, expressions_only=True, allow_recovery=True)
            test.assertNotIn("accepted", model.expression_validity.values())


def check_rejection(test):
    source = test.source("print 1 +\nprint 2\n", "recovery-marker.bas")
    _, artifact = test.invoke([source], mode="expressions", success=False)
    model = Model.read(artifact, expressions_only=True, allow_recovery=True)
    original = [row.copy() for row in model.rows]
    for mutation in ("missing-marker", "duplicate-marker"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in original]
            if mutation == "missing-marker":
                rows = [row for row in rows if row[0] not in ("R", "RECOVERY")]
                counts = Counter(row[0] for row in rows)
                rows.append(["END", "27", *map(str, (counts[tag] for tag in ("M", "P", "S", "V", "N", "E", "B", "I", "D"))),
                             "1", str(sum(counts[tag] for tag in DETAIL_TAGS))])
            else:
                rows.insert(-1, next(row.copy() for row in rows if row[0] == "R"))
                rows[-1][4] = "2"
            with test.assertRaises(ValueError):
                Model(encoded(rows), expressions_only=True, allow_recovery=True)

def check_modules(test):
    valid = test.source("print 1\n", "valid-first.bas")
    failed = test.source("print 1 +\nprint 2\n", "failed-second.bas")
    unprocessed = test.source("print 3 +\n", "unprocessed-third.bas")
    _, artifact = test.invoke([valid, failed, unprocessed], mode="expressions", success=False)
    model = Model.read(artifact, expressions_only=True, allow_recovery=True)
    test.assertEqual(model.recovered_modules, {2})
    test.assertEqual(model.module_validity, {1: "accepted", 2: "recovery"})
    test.assertEqual(len(model.records["M"]), 2)
    test.assertNotIn(str(unprocessed), {row[1] for row in model.records["D"]})
    restarted = test.source('#lang "fblite"\nprint 1\n', "validity-restarted.bas")
    model = test.compile(restarted, mode="expressions")
    test.assertEqual(model.module_validity, {1: "accepted"})
    test.assertFalse(model.recovered_modules)
    test.assertEqual(set(model.expression_validity.values()), {"accepted"})

# end of recovery_validity.py
