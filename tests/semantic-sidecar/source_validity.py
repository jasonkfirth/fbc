"""Project: FreeBASIC source validity tests
File: source_validity.py
Purpose: Exercise native statement outcomes and explicit failure boundaries.
Responsibilities: Parsed/recovered regions, unfinished compounds and report-only loci.
This file intentionally does NOT certify expressions in a rejected module.
"""

from driver_inputs import text_rows
from recovery_steps import recovered
from recovery_validity import encoded
from sidecar import Model


def check_regions(test):
    source = test.source("dim value as long = 1\nprint 1 +\nprint value\n", "native-validity-middle.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
            result, model = recovered(test, source, backend)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            test.assertEqual(model.source_validity[1][0], "native-parser")
            statements = {int(row[13]): identity for identity, row in model.statements.items()}
            statuses = [model.native_statement_closes[statements[line]][3] for line in (1, 2, 3)]
            test.assertEqual(statuses, ["parsed", "recovered", "parsed"])
            test.assertEqual(model.native_statement_closes[statements[2]][:2], ["0", "1"])
            region = next(region for region in model.failure_regions if region["domain"] == "statement")
            test.assertEqual(region["identity"], statements[2])
            test.assertEqual(region["status"], "recovered")
            test.assertTrue(region["diagnostics"])
            test.assertNotIn("accepted", model.expression_validity.values())
    missing = test.source('print 1\n#include "missing-source-validity.bi"\nprint 2\n', "native-validity-include.bas")
    _, model = recovered(test, missing)
    diagnostic = next(region for region in model.failure_regions if region["domain"] == "diagnostic")
    test.assertEqual(diagnostic["line"], 2)
    test.assertIsNone(diagnostic["extent"])


def check_unfinished(test):
    source = test.source("scope\ndim value as long = 1\n", "native-validity-unfinished.bas")
    _, model = recovered(test, source)
    regions = [region for region in model.failure_regions if region["domain"] == "construct"]
    test.assertTrue(regions)
    test.assertTrue(all(region["status"] == "unfinished" and region["end"] is None for region in regions))
    test.assertGreater(int(model.source_validity[1][6]), 0)
    valid = test.source("scope\ndim value as long = 1\nend scope\n", "native-validity-complete.bas")
    for mode in ("full", "bindings", "expressions"):
        model = test.compile(valid, mode=mode)
        test.assertFalse(model.failure_regions)
        test.assertEqual(model.source_validity[1][3], "0")
        test.assertEqual(model.source_validity[1][6], "0")


def check_rejection(test):
    source = test.source("print 1 +\nprint 2\n", "native-validity-corruption.bas")
    _, model = recovered(test, source)
    for mutation in ("opening", "closing", "summary", "count", "phase", "counter", "outcome", "identity", "duplicate"):
        with test.subTest(validity_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            opening = next(row for row in rows if row[3:4] and row[3].startswith("native-statement-open:"))
            closing = next(row for row in rows if row[3:4] and row[3].startswith("native-statement-close:"))
            summary = next(row for row in rows if row[3:4] == ["native-source-validity-counts"])
            if mutation in ("opening", "closing", "summary"):
                rows.remove({"opening": opening, "closing": closing, "summary": summary}[mutation])
            elif mutation == "identity":
                closing[3] = "native-statement-close:999999"
            elif mutation == "duplicate":
                rows.insert(-1, opening.copy())
            elif mutation in ("count", "phase"):
                fields = summary[4].split("\t")
                fields[1 if mutation == "count" else 0] = "0" if mutation == "count" else "late-backend"
                summary[4] = "\t".join(fields)
            else:
                fields = closing[4].split("\t")
                fields[1 if mutation == "counter" else 3] = "0" if mutation == "counter" else "parsed"
                closing[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(encoded(rows), expressions_only=True, allow_recovery=True)
    valid = test.source("print 1\n", "native-validity-zero-count.bas")
    model = test.compile(valid)
    rows = [row.copy() for row in model.rows if row[3:4] != ["native-source-validity-counts"]]
    with test.assertRaises(ValueError):
        Model(text_rows(rows))

# end of source_validity.py
