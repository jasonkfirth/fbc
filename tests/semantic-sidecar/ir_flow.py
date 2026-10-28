"""Project: FreeBASIC semantic sidecar tests
File: ir_flow.py
Purpose: Prove actual IR transfer coverage and conservative graph membership.
Responsibilities: Nested/late/legacy transfers, continuations, uncertainty and corruption.
This file intentionally does NOT parse source text to reconstruct a CFG.
"""

import subprocess
from pathlib import Path

from driver_inputs import text_rows
from semantic_ir_flow import FEATURE, KINDS
from sidecar import Model


def graphs(model):
    return model.native_ir_flow[1]["graphs"].values()


def check_graphs(test):
    observed = set()
    assembly = test.source("sub IRFlowAssembly()\nasm\nnop\nend asm\nend sub\n", "ir-flow-assembly.bas")
    choices = [(backend, test.fixture(name), ("-exx",)) for backend in test.backends
               for name in ("ir-control-flow.bas", "ir-control-legacy.bas", "ir-control-error.bas")]
    choices += [(backend, assembly, ()) for backend in test.backends]
    choices += [("gas64", test.fixture("ir-control-legacy.bas"), ("-exx",)),
                ("gas", test.fixture("ir-control-legacy.bas"), ("-exx", "-target", "linux-x86"))]
    for backend, source, extra in choices:
        with test.subTest(backend=backend, fixture=source.name):
            test.invoke([source], mode="off", backend=backend, extra=extra)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertTrue(model.native_ir_flow[1]["graphs"])
            for graph in graphs(model):
                test.assertEqual(graph["header"][2], backend)
                observed.update(row[0] for row in graph["actions"].values())
                test.assertEqual(len(graph["actions"]), graph["end"][0])
                test.assertEqual(len(graph["blocks"]), graph["end"][1])
                test.assertEqual(len(graph["edges"]), graph["end"][2])
                test.assertTrue(all(row[2] != "executed" for row in graph["blocks"].values()))
            if source.name == "ir-control-flow.bas":
                unused = test.one(model, "IRFlowUnused", "procedure")
                test.assertFalse(any(graph["header"][0] == str(unused) for graph in graphs(model)))
                test.assertTrue(any(action[0] == "conditional" for graph in graphs(model) for action in graph["actions"].values()))
                # Bounds/pointer guard labels are created after the pre-load
                # snapshot, and still have real definitions in the IR graph.
                snapshot_labels = {row[2] for row in model.records["CL"]}
                emitted_labels = {action[4] for graph in graphs(model) for action in graph["actions"].values() if action[0] == "label"}
                test.assertTrue(emitted_labels - snapshot_labels)
            if source.name == "ir-control-error.bas":
                test.assertTrue(any(graph["addresses"] for graph in graphs(model)))
                test.assertTrue(any(edge[2] == "unknown-indirect" for graph in graphs(model) for edge in graph["edges"].values()))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
    test.assertEqual(observed, KINDS)


def check_return_policies(test):
    source = test.source('declare sub UserExitAlias alias "fb_End"(byval value as long)\n'
                         'sub AliasCaller()\nUserExitAlias(0)\nprint "may-return"\nend sub\n'
                         'sub KnownIRExit()\nend 3\nprint "unreachable"\nend sub\n'
                         'function IRValueReturn() as integer\nreturn 7\nend function\n', "ir-return-policies.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            external = test.one(model, "UserExitAlias", "procedure")
            calls = [action for graph in graphs(model) for action in graph["actions"].values() if action[0] == "call"]
            test.assertTrue(any(row[5] == str(external) and row[6] == "may-return" for row in calls))
            test.assertTrue(any(row[6] in ("runtime-end", "known-no-return") for row in calls))
            test.assertTrue(any(action[0] == "return-value" for graph in graphs(model) for action in graph["actions"].values()))
            for graph in graphs(model):
                successors = {}
                for edge in graph["edges"].values():
                    successors.setdefault(edge[0], []).append(edge)
                for action in graph["actions"].values():
                    if action[0] in ("jump", "return-value", "indirect", "subroutine-return", "switch") or action[0] == "call" and action[6] != "may-return":
                        test.assertFalse(any(edge[2] in ("fallthrough", "call-continuation") for edge in successors[action[1]]))


def check_modes(test):
    source = test.fixture("ir-control-flow.bas")
    for mode in ("bindings", "expressions"):
        model = test.compile(source, mode=mode, extra=("-exx",))
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertFalse(model.native_ir_flow)
    for abi in ("native", "stable-v1"):
        text = test.compile(source, extra=("-exx", "-semantic-type-abi", abi))
        artifact = test.working / ("ir-flow-" + abi + ".bin")
        test.invoke([source], mode="off", extra=("-exx", "-semantic-type-abi", abi, "-semantic-model", str(artifact)))
        test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source("sub FirstIRGraph()\nprint 1\nend sub\n", "first-ir-graph.bas")
    second = test.source("sub SecondIRGraph()\nprint 2\nend sub\n", "second-ir-graph.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    test.assertEqual(set(model.native_ir_flow), {1, 2})
    test.assertTrue(all(1 in state["graphs"] for state in model.native_ir_flow.values()))


def check_rejection(test):
    model = test.compile(test.fixture("ir-control-flow.bas"), extra=("-exx",))
    for mutation in ("owner", "action-kind", "block", "load", "target", "callee", "return-policy", "case-target", "case-value",
                     "edge-target", "edge-kind", "reachability", "procedure-count", "unavailable", "recounted-edge", "recounted-action", "recounted-label"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def packet(prefix):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            if mutation == "owner":
                packet("native-ir-procedure:")[2] = "0"
            elif mutation in ("action-kind", "block", "load", "target", "callee"):
                row = packet("native-ir-action:");fields = row[4].split("\t")
                fields[{"action-kind": 0, "block": 1, "load": 2, "target": 4, "callee": 5}[mutation]] = "executed" if mutation == "action-kind" else "4294967295"
                row[4] = "\t".join(fields)
            elif mutation == "return-policy":
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("native-ir-action:") and row[4].split("\t")[0] == "call")
                fields = row[4].split("\t");fields[6] = "runtime-end" if fields[6] != "runtime-end" else "may-return";row[4] = "\t".join(fields)
            elif mutation in ("case-target", "case-value"):
                row = packet("native-ir-case:");fields = row[4].split("\t");fields[1 if mutation == "case-target" else 0] = "0" if mutation == "case-target" else "18446744073709551616";row[4] = "\t".join(fields)
            elif mutation.startswith("edge-"):
                row = packet("native-ir-edge:");fields = row[4].split("\t");fields[1 if mutation == "edge-target" else 2] = "1000000" if mutation == "edge-target" else "executed";row[4] = "\t".join(fields)
            elif mutation == "reachability":
                row = packet("native-ir-block:");fields = row[4].split("\t");fields[2] = "unreachable";row[4] = "\t".join(fields)
            elif mutation == "procedure-count":
                row = packet("native-ir-counts:");fields = row[4].split("\t");fields[0] = "0";row[4] = "\t".join(fields)
            elif mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            else:
                family = {"recounted-edge": "edge", "recounted-action": "action", "recounted-label": "action"}[mutation]
                candidates = [row for row in rows if row[0] == "K" and row[3].startswith("native-ir-" + family + ":")]
                victim = next(row for row in candidates if mutation != "recounted-label" or row[4].split("\t")[0] == "label")
                graph = victim[3].split(":")[1];rows.remove(victim)
                remaining = [row for row in candidates if row is not victim and row[3].split(":")[1] == graph]
                for ordinal, row in enumerate(remaining, 1):row[3] = "native-ir-" + family + ":" + graph + ":" + str(ordinal)
                slot = 2 if family == "edge" else 0
                ending = next(row for row in rows if row[0] == "K" and row[3] == "native-ir-end:" + graph)
                fields = ending[4].split("\t");fields[slot] = str(int(fields[slot]) - 1);ending[4] = "\t".join(fields)
                closure = packet("native-ir-counts:");fields = closure[4].split("\t");fields[slot + 1] = str(int(fields[slot + 1]) - 1);closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):Model(text_rows(rows))


def check_execution(test):
    expected = {"ir-control-flow.bas": "16", "ir-control-legacy.bas": "legacy-ok", "ir-control-error.bas": "recovered"}
    for filename, output in expected.items():
        for mode in ("off", "full", "bindings", "expressions"):
            with test.subTest(fixture=filename, mode=mode):
                executable = test.working / (Path(filename).stem + "-" + mode)
                test.invoke([test.fixture(filename)], mode=mode, emit=False, extra=("-exx", "-x", str(executable)))
                result = subprocess.run([str(executable)], text=True, capture_output=True, timeout=30)
                test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                test.assertEqual(result.stdout.strip(), output)


def check_guards(test):
    executable = test.working / "ir-flow-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               "-i", str(test.root / "inc"), str(test.fixture("ir-flow-guards.bas")), str(test.root / "src/compiler/tooling/semantic-ir-flow.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, text=True, capture_output=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], text=True, capture_output=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "IR_FLOW_GUARDS_PASS")

# end of ir_flow.py
