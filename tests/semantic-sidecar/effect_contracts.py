"""Project: FreeBASIC semantic sidecar tests
File: effect_contracts.py
Purpose: Prove selected runtime contracts without guessing effects from names.
Responsibilities: Domains, uncertainty, aliases, indirect calls and native parity.
This file intentionally does NOT treat cloneability as a purity certificate.
"""

import subprocess

from driver_inputs import text_rows
from semantic_effects import FEATURE, DOMAINS, EFFECTS
from sidecar import Model


def check_contracts(test):
    observed = set()
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.fixture("effects.bas")
            test.invoke([source], mode="off", backend=backend, extra=("-exx",))
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=("-exx",))
            state = model.effect_contracts[1]
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertEqual(len(state["domains"]), len(DOMAINS))
            test.assertEqual(len(state["definitions"]), len(EFFECTS))
            observed.update(row[1][1] for row in state["declarations"].values())
            named = {model.symbol_name(selected).casefold(): fields for selected, fields in state["declarations"].values()}
            for name in ("UnregisteredAllocator", "UnregisteredMath"):
                fields = named[name.casefold()]
                test.assertEqual(fields[:4], ["unknown-callee", "unknown", "1", "unknown"])
            indirect = [row for row in state["calls"].values() if row[4] == "1"]
            test.assertTrue(indirect)
            test.assertTrue(all(row[5:] == ["indirect-target", "unknown", "1", "unknown", "unknown-remainder", "0", "unknown-aliases"] for row in indirect))
            contracts = [fields for _, fields in state["declarations"].values()]
            for name in ("allocate", "release", "argument-write", "synchronization", "error-handling", "process-exit"):
                test.assertTrue(any(name in row[3].split(",") for row in contracts), name)
            test.assertTrue(any(row[1] == "math" and row[2] == "8192" for row in contracts))
            # A null check can be native-cloneable while retaining error and
            # control-transfer effects. The two facts must not be conflated.
            test.assertTrue(any(row[7] == "1" and "error-handling" in row[3].split(",") and row[2] != "8192" for row in contracts))
            test.assertTrue(any(row[3] == "assembly" and row[6] == "unknown-memory-registers-control" for row in state["operations"].values()))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
    test.assertTrue({"memory", "math", "string", "unicode", "array", "file", "print", "graphics", "sound", "system", "error"} <= observed, observed)


def check_modes(test):
    source = test.fixture("load-actions.bas")
    for mode in ("bindings", "expressions"):
        model = test.compile(source, mode=mode, extra=("-exx",))
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertFalse(model.effect_contracts)
    for abi in ("native", "stable-v1"):
        text = test.compile(source, extra=("-exx", "-semantic-type-abi", abi))
        artifact = test.working / ("effects-" + abi + ".bin")
        test.invoke([source], mode="off", extra=("-exx", "-semantic-type-abi", abi, "-semantic-model", str(artifact)))
        test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source('print "first"\n', "first-effects.bas")
    second = test.source('print "second"\n', "second-effects.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    test.assertEqual(set(model.effect_contracts), {1, 2})
    test.assertTrue(all(1 in state["calls"] for state in model.effect_contracts.values()))


def check_rejection(test):
    model = test.compile(test.fixture("effects.bas"), extra=("-exx",))
    for mutation in ("origin", "domain", "mask", "names", "certainty", "aliases", "cloneable", "indirect", "call-owner", "operation", "dictionary", "unavailable", "recounted-call", "recounted-operation", "recounted-declaration"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def packet(prefix):return next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            if mutation in ("origin", "domain", "mask", "names", "certainty", "aliases", "cloneable"):
                row = packet("effect-declaration:")
                fields = row[4].split("\t")
                slots = {"origin": (0, "name-guess"), "domain": (1, "unrecognized"), "mask": (2, "0"), "names": (3, "pure"),
                         "certainty": (4, "guaranteed-executed"), "aliases": (6, "exact-all-memory"), "cloneable": (7, "2")}
                slot, value = slots[mutation];fields[slot] = value;row[4] = "\t".join(fields)
            elif mutation == "indirect":
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("effect-call:") and row[4].split("\t")[4] == "1")
                fields = row[4].split("\t");fields[5:] = ["runtime-registration", "math", "8192", "computation", "known-potential", "0", "unknown-aliases"];row[4] = "\t".join(fields)
            elif mutation == "call-owner":
                row = packet("effect-call:");fields = row[4].split("\t");fields[1] = "1000000";row[4] = "\t".join(fields)
            elif mutation == "operation":
                row = packet("effect-operation:");fields = row[4].split("\t");fields[6] = "pure";row[4] = "\t".join(fields)
            elif mutation == "dictionary":packet("effect-definition:")[4] = "1\tpure"
            elif mutation == "unavailable":next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            else:
                family = {"recounted-call": "call", "recounted-operation": "operation", "recounted-declaration": "declaration"}[mutation]
                items = [row for row in rows if row[0] == "K" and row[3].startswith("effect-" + family + ":")]
                rows.remove(items[-1])
                closure = packet("effect-counts:");fields = closure[4].split("\t");slot = {"declaration": 0, "call": 1, "operation": 2}[family];fields[slot] = str(int(fields[slot]) - 1);closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):Model(text_rows(rows))


def check_profile_and_execution(test):
    source = test.source('sub ProfileEffect()\nprint "effects-ok"\nend sub\nProfileEffect()\n', "profile-effects.bas")
    model = test.compile(source, extra=("-profile",))
    test.assertTrue(any(fields[1] == "profile" and "profiling" in fields[3].split(",") for _, fields in model.effect_contracts[1]["declarations"].values()))
    runtime = test.fixture("load-actions.bas")
    outputs = []
    for mode in ("off", "full", "bindings", "expressions"):
        executable = test.working / ("effects-runtime-" + mode)
        test.invoke([runtime], mode=mode, emit=False, extra=("-exx", "-x", str(executable)))
        result = subprocess.run([str(executable)], text=True, capture_output=True, timeout=30)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        outputs.append(result.stdout)
    test.assertTrue(all(output == outputs[0] for output in outputs))


def check_guards(test):
    executable = test.working / "effect-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("effect-guards.bas")), str(test.root / "src/compiler/tooling/semantic-effects.bas"),
               str(test.root / "src/compiler/runtime/rtl-effects.bas"), "-x", str(executable)]
    result = subprocess.run(command, text=True, capture_output=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], text=True, capture_output=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "EFFECT_GUARDS_PASS")

# end of effect_contracts.py
