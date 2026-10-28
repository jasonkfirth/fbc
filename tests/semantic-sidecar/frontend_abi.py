"""Project: FreeBASIC semantic sidecar tests
File: frontend_abi.py
Purpose: Prove frontend ABI meaning and reject contradictory snapshots.
Responsibilities: Modes, live signatures, omissions, target widths and closure.
This file intentionally does NOT inspect generated register allocation.
"""

import subprocess

from driver_inputs import text_rows
from semantic_frontend_abi import FEATURE
from sidecar import Model


def parameters(model, identity):
    return {int(fields[1]): (owner, fields) for owner, fields in model.frontend_abi[1]["parameter"].values()
            if fields[0] == str(identity)}


def procedure(model, identity):
    return next(fields for owner, fields in model.frontend_abi[1]["procedure"].values() if owner == identity)


def check_modes(test):
    source = test.fixture("frontend-abi.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            value = parameters(model, test.one(model, "ValueABI", "procedure"))
            test.assertEqual([value[index][1][12] for index in range(3)], ["value", "implicit-reference", "implicit-reference"])
            reference = parameters(model, test.one(model, "ReferenceABI", "procedure"))
            test.assertEqual(reference[0][1][3], "byref")
            test.assertEqual(reference[0][1][12], "reference")
            test.assertEqual(reference[1][1][3], "bydesc")
            test.assertEqual(reference[1][1][12], "descriptor")
            test.assertEqual(reference[1][1][7], reference[1][1][10])
            variadic = parameters(model, test.one(model, "VariadicABI", "procedure"))
            test.assertEqual(variadic[1][1][2], "variadic")
            test.assertEqual(variadic[1][1][6:9], ("", "", "0"))
            result = procedure(model, test.one(model, "ReferenceResultABI", "procedure"))
            test.assertEqual(result[0], "available")
            test.assertEqual(result[5], "byref")
            large = procedure(model, test.one(model, "LargeResultABI", "procedure"))
            # High-level backends retain an aggregate value in the frontend.
            # A later C/LLVM ABI can still select caller-provided storage.
            test.assertEqual(large[5], "value")
            methods = model.named("ChangeValue", "procedure")
            test.assertTrue(methods)
            method = parameters(model, methods[0])
            test.assertIn(-1, method)
            test.assertEqual(method[-1][1][11], "1")
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            for mode in ("bindings", "expressions"):
                reduced = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(reduced.frontend_abi)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
    for backend, target in (("gas", "linux-x86"), ("gas64", "linux-x86_64")):
        with test.subTest(backend=backend, target=target):
            model = test.compile(source, backend=backend, extra=("-target", target))
            large = procedure(model, test.one(model, "LargeResultABI", "procedure"))
            test.assertEqual(large[5], "hidden")


def check_targets(test):
    source = test.fixture("frontend-abi.bas")
    for backend in test.backends:
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            with test.subTest(backend=backend, target=target):
                model = test.compile(source, backend=backend, extra=("-target", target))
                width = "8" if target.endswith("x86_64") else "4"
                scalar = parameters(model, test.one(model, "ScalarABI", "procedure"))[0][1]
                test.assertEqual(scalar[8], width)
                test.assertEqual(scalar[9], "0")
                reference = parameters(model, test.one(model, "ReferenceABI", "procedure"))
                test.assertEqual([reference[index][1][8] for index in (0, 1)], [width, width])
                test.assertTrue(all(fields[2] != "available" or fields[6] != ""
                                    for owner, fields in model.frontend_abi[1]["parameter"].values()))


def check_transport(test):
    source = test.fixture("frontend-abi.bas")
    text = test.compile(source)
    artifact = test.working / "frontend-abi.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(artifact), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source("declare sub FirstABI(byval value as long)\n", "first-abi.bas")
    second = test.source("declare sub SecondABI(byref value as double)\n", "second-abi.bas")
    _, artifact = test.invoke([first, second])
    multiple = Model.read(artifact)
    test.assertEqual(set(multiple.frontend_abi), {1, 2})
    test.assertEqual(list(multiple.frontend_abi[2]["procedure"])[0], 1)
    test.assertEqual(list(multiple.frontend_abi[2]["parameter"])[0], 1)


def check_rejection(test):
    model = test.compile(test.fixture("frontend-abi.bas"))
    for mutation in ("procedure-state", "procedure-type", "procedure-kind", "parameter-owner", "ordinal",
                     "mode", "state", "source", "lowered", "descriptor", "slot", "register", "instance",
                     "classification", "variadic", "missing-procedure", "missing-parameter", "count", "unavailable"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            if mutation.startswith("procedure-"):
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("frontend-procedure-abi:"))
                fields = row[4].split("\t")
                slot = {"procedure-state": 0, "procedure-type": 3, "procedure-kind": 5}[mutation]
                fields[slot] = {"procedure-state": "invented", "procedure-type": "2147483648", "procedure-kind": "invented"}[mutation]
            elif mutation == "count":
                row = next(row for row in rows if row[0] == "K" and row[3] == "frontend-abi-counts:1")
                fields = ["0", "0"]
            elif mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
                Model(text_rows(rows))
                continue
            elif mutation.startswith("missing-"):
                family = mutation.split("-")[1]
                rows.remove(next(row for row in rows if row[0] == "K" and row[3].startswith("frontend-" + family + "-abi:")))
                Model(text_rows(rows))
                continue
            else:
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("frontend-parameter-abi:")
                           and (mutation != "variadic" or row[4].split("\t")[2] == "variadic")
                           and (mutation != "descriptor" or row[4].split("\t")[3] == "bydesc")
                           and (mutation != "slot" or row[4].split("\t")[0] == str(test.one(model, "ScalarABI", "procedure"))))
                fields = row[4].split("\t")
                if mutation == "parameter-owner":
                    row[2] = fields[0]
                elif mutation == "ordinal":
                    row[3] += ":invalid"
                else:
                    slot = {"mode": 3, "state": 2, "source": 4, "lowered": 6, "descriptor": 10,
                            "slot": 8, "register": 9, "instance": 11, "classification": 12, "variadic": 6}[mutation]
                    fields[slot] = {"mode": "vararg", "state": "invented", "source": "999999", "lowered": "999999",
                                    "descriptor": "0", "slot": "999999", "register": "3", "instance": "2",
                                    "classification": "invented", "variadic": "8"}[mutation]
            row[4] = "\t".join(fields)
            Model(text_rows(rows))


def check_guards(test):
    executable = test.working / "abi-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("frontend-abi-guards.bas")), str(test.root / "src/compiler/tooling/semantic-abi.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "FRONTEND_ABI_GUARDS_PASS")

# end of frontend_abi.py
