"""Project: FreeBASIC semantic sidecar tests
File: object_operations.py
Purpose: Prove selected construction, copying, destruction and excluded actions.
Responsibilities: Native routes, roots, ordering, transport, runtime and corruption.
This file intentionally does NOT reconstruct resolver choices from type names.
"""

import subprocess

from driver_inputs import text_rows
from semantic_object_operations import FEATURE, BASE_FEATURE, KINDS
from sidecar import Model


def cpp_source(test):
    return test.source('extern "c++"\n'
                       'type OperationCppBase extends object\n'
                       'declare virtual destructor()\nend type\n'
                       'type OperationCppChild extends OperationCppBase\n'
                       'payload as string\nend type\nend extern\n'
                       'destructor OperationCppBase()\nend destructor\n'
                       'dim instance as OperationCppChild\n', "cpp-object-operations.bas")


def check_operations(test):
    observed = set()
    for backend in test.backends:
        for source in (test.fixture("object-operations.bas"), cpp_source(test)):
            with test.subTest(backend=backend, fixture=source.name):
                test.invoke([source], mode="off", backend=backend)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                test.assertEqual(model.capabilities[1][BASE_FEATURE], "available")
                facts = model.native_object_operations[1]
                test.assertTrue(facts["operations"])
                observed.update(row[1] for row in facts["operations"].values())
                test.assertEqual(set(facts["operations"]), set(facts["ends"]))
                test.assertEqual(set(facts["operations"]), set(facts["node_counts"]))
                test.assertTrue(any(facts["projections"].values()))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                if source.name != "object-operations.bas":
                    continue
                owner = test.one(model, "OperationDerived", "type")
                base = model.base_subobject_fields[owner]
                test.assertEqual(model.symbols[base][3], "12")
                test.assertEqual(model.symbols[base][5], model.layouts[owner][3])
                constructors = {row[2] for row in facts["operations"].values()
                                if row[1] == "constructor-initialization" and row[3] == str(owner)}
                chains = [(identity, row) for identity, row in facts["operations"].items()
                          if row[1] == "constructor-chain" and row[3] == str(owner)]
                test.assertEqual(len(chains), 1)
                test.assertNotIn(chains[0][1][2], constructors)
                test.assertNotEqual(facts["selections"][chains[0][0]][0], "0")
                policies = [row for rows in facts["choices"].values() for row in rows]
                for choice in ("default-constructor", "default-constructor-array", "no-initialize",
                               "fake-dynamic-skip", "zero-bitfield", "zero-storage"):
                    test.assertIn(["field-init", choice], policies)
                for identity, row in facts["operations"].items():
                    if ["field-init", "no-initialize"] in facts["choices"][identity]:
                        test.assertEqual(facts["ends"][identity][0], "0")
                        test.assertFalse(facts["projections"][identity])
                plain = test.one(model, "OperationPlain", "type")
                test.assertFalse(any(row[3] == str(plain) and row[1] in
                                     ("default-constructor-call", "implicit-construction", "reused-construction")
                                     and row[5] != "0" for row in facts["operations"].values()))
                unused = test.one(model, "OperationUnused", "procedure")
                test.assertTrue(any(row[2] == str(unused) for row in facts["operations"].values()))
                phase = next(row for row in model.records["PH"] if row[2] == str(unused))
                test.assertEqual(phase[4], "0")
                assigned = test.one(model, "OperationAssignedResult", "procedure")
                returned = test.one(model, "OperationReturnedResult", "procedure")
                initialized_results = {row[2] for row in facts["operations"].values() if row[1] == "result-initialization"}
                test.assertIn(str(assigned), initialized_results)
                test.assertNotIn(str(returned), initialized_results)
    test.assertEqual(observed, KINDS)


def check_modes(test):
    source = test.fixture("object-operations.bas")
    for mode in ("bindings", "expressions"):
        model = test.compile(source, mode=mode)
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertEqual(model.capabilities[1][BASE_FEATURE], "unavailable")
        test.assertFalse(model.native_object_operations)
        test.assertFalse(model.base_subobject_fields)
    for abi in ("native", "stable-v1"):
        text = test.compile(source, extra=("-semantic-type-abi", abi))
        artifact = test.working / ("object-operations-" + abi + ".bin")
        test.invoke([source], mode="off", extra=("-semantic-type-abi", abi, "-semantic-model", str(artifact)))
        test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source('type FirstOperation\ntext as string\nend type\ndim first as FirstOperation\n', "first-operation.bas")
    second = test.source('type SecondOperation\ntext as string\nend type\ndim second as SecondOperation\n', "second-operation.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    test.assertEqual(set(model.native_object_operations), {1, 2})
    for facts in model.native_object_operations.values():
        test.assertTrue(facts["operations"])
        test.assertEqual(facts["operations"][1][0], "0")
    empty = test.source("print 1\n", "no-object-operation.bas")
    facts = test.compile(empty).native_object_operations[1]
    test.assertEqual(facts["closure"], [0, 0, 0, 0, 0])


def check_rejection(test):
    model = test.compile(test.fixture("object-operations.bas"))
    for mutation in ("parent", "kind", "owner", "target", "selected", "mode", "decision", "statement", "source", "configuration",
                     "end-class", "end-outcome", "choice", "selection", "root-link", "base-field", "unavailable",
                     "recounted-choice", "recounted-root", "recounted-operation", "missing-base"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def packet(prefix, domain="symbol"):
                return next(row for row in rows if row[0] == "K" and row[1] == domain and row[3].startswith(prefix))
            if mutation in ("parent", "kind", "owner", "target", "selected", "mode", "decision", "statement", "source", "configuration"):
                row = packet("native-object-operation:")
                slots = {"parent": 0, "kind": 1, "owner": 3, "target": 4, "selected": 5, "mode": 6,
                         "decision": 7, "statement": 8, "source": 9, "configuration": 10}
                fields = row[4].split("\t")
                fields[slots[mutation]] = "unknown-operation" if mutation == "kind" else "4294967296"
                row[4] = "\t".join(fields)
            elif mutation.startswith("end-"):
                row = packet("native-object-end:")
                fields = row[4].split("\t")
                fields[1 if mutation == "end-class" else 2] = "999999" if mutation == "end-class" else "executed"
                row[4] = "\t".join(fields)
            elif mutation == "choice":
                packet("native-object-choice:")[4] = "field-init\tguaranteed-pure"
            elif mutation == "selection":
                packet("native-object-selection:")[4] = "0\t9"
            elif mutation == "root-link":
                packet("native-object-operation:", "node")[3] = "native-object-operation:1000000"
            elif mutation == "base-field":
                row = next(row for row in rows if row[0] == "H" and row[5] == "base-field")
                row[4] = "0"
            elif mutation == "missing-base":
                owner = next(row[2] for row in rows if row[0] == "H" and row[5] == "base-field")
                rows = [row for row in rows if not (row[0] == "H" and row[2] == owner and row[5] == "base-field")]
            elif mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            elif mutation == "recounted-choice":
                choice = packet("native-object-choice:")
                identity = choice[3].split(":")[1]
                rows.remove(choice)
                ending = next(row for row in rows if row[0] == "K" and row[3] == "native-object-end:" + identity)
                fields = ending[4].split("\t");fields[3] = str(int(fields[3]) - 1);ending[4] = "\t".join(fields)
                closure = packet("native-object-counts:")
                fields = closure[4].split("\t");fields[2] = str(int(fields[2]) - 1);closure[4] = "\t".join(fields)
            elif mutation == "recounted-root":
                root = packet("native-object-operation:", "node")
                rows.remove(root)
                # Keep the independent per-event root count to reject loss even
                # when an attacker repairs the module and transport totals.
                closure = packet("native-object-counts:")
                fields = closure[4].split("\t");fields[4] = str(int(fields[4]) - 1);closure[4] = "\t".join(fields)
            else:
                ending = next(row for row in reversed(rows) if row[0] == "K" and row[3].startswith("native-object-end:"))
                rows.remove(ending)
                closure = packet("native-object-counts:")
                fields = closure[4].split("\t");fields[1] = str(int(fields[1]) - 1);closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_execution(test):
    for mode in ("off", "full", "bindings", "expressions"):
        with test.subTest(mode=mode):
            executable = test.working / ("object-operations-" + mode)
            test.invoke([test.fixture("object-operations.bas")], mode=mode, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertEqual(result.stdout.replace(" ", "").strip(), "5:7:returned:assigned:9")


def check_guards(test):
    executable = test.working / "object-operation-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("object-operation-guards.bas")),
               str(test.root / "src/compiler/tooling/semantic-object-operations.bas"), "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "OBJECT_OPERATION_GUARDS_PASS")


def check_plain_storage(test):
    for expression in ("original", "(original)"):
        source = test.source("type PlainStorage\nvalue as long\nend type\n"
                             "dim original as PlainStorage\noriginal.value = 9\n"
                             "dim copied as PlainStorage = " + expression + "\nprint copied.value\n",
                             "plain-object-storage-" + str(len(expression)) + ".bas")
        for backend in test.backends:
            with test.subTest(backend=backend, expression=expression):
                test.invoke([source], mode="off", backend=backend)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                facts = model.native_object_operations[1]
                if expression == "original":
                    test.assertEqual(facts["closure"], [0, 0, 0, 0, 0])
                    test.assertFalse(facts["operations"])
                else:
                    test.assertEqual(len(facts["operations"]), 1)
                    identity, event = next(iter(facts["operations"].items()))
                    test.assertEqual(event[1], "implicit-construction")
                    test.assertEqual(event[5], "0")
                    test.assertEqual(facts["ends"][identity][2], "shallow")
                    test.assertFalse(facts["choices"][identity])
                    rows = [row.copy() for row in model.rows]
                    ending = next(row for row in rows if row[0] == "K" and row[3] == "native-object-end:" + str(identity))
                    fields = ending[4].split("\t");fields[2] = "built";ending[4] = "\t".join(fields)
                    with test.assertRaises(ValueError):
                        Model(text_rows(rows))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)

# end of object_operations.py
