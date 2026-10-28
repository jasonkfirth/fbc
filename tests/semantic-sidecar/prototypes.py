"""Project: FreeBASIC semantic sidecar tests
File: prototypes.py
Purpose: Check actual native declaration/prototype comparison receipts.
Responsibilities: Native helper guards, arity/type/nominal/property cases and corruption.
This file intentionally does NOT infer executed calls from matched declarations.
"""

import subprocess

from driver_inputs import text_rows
from recovery_steps import recovered
from semantic_prototypes import FEATURE
from sidecar import Model

MATCHES = '''declare sub Work overload(byval first as long, byval second as double)
declare sub Work overload(byval first as double, byval second as long)
declare sub Work overload()
sub Work(byval first as double, byval second as long)
end sub
sub Work(byval first as long, byval second as double)
end sub
sub Work()
end sub
Work(1l, 2.0)
'''


def check_guards(test):
    native = (test.root / "src/compiler/symbols/symb-proc.bas").read_text()
    bodies = []
    for name in ("hCanOverloadBydesc", "symbFindOverloadProc"):
        start = native.index(("private " if name.startswith("h") else "") + "function " + name + " _")
        end = native.index("\nend function", start) + len("\nend function")
        bodies.append(native[start:end])
    guard = (test.root / "tests/semantic-sidecar/prototypes-guards.bas").read_text()
    index = guard.index("lex.ctx = @lex.ctxTB(0)")
    source = test.working / "prototypes-native-guards.bas"
    source.write_text(guard[:index] + "\n".join(bodies) + "\n\n" + guard[index:])
    executable = test.working / ("prototypes-guards.exe" if test.native_windows else "prototypes-guards")
    collector = test.working / "semantic-prototypes.bas"
    collector.write_bytes((test.root / "src/compiler/tooling/semantic-prototypes.bas").read_bytes())
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", source.stem, "-x", str(executable), str(source),
                             str(collector)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("PROTOTYPE_GUARDS_PASS", result.stdout)


def check_matches(test):
    source = test.source(MATCHES, "prototype-matches.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        scopes = list(model.prototype_scopes.values())
        test.assertTrue(scopes)
        candidates = [candidate for scope in scopes for candidate in scope["candidates"]]
        test.assertTrue(any(candidate["event"][3] != candidate["event"][5] for candidate in candidates))
        test.assertTrue(any(parameter[4:6] == ["type", "1"] for candidate in candidates for parameter in candidate["parameters"]))
        test.assertTrue(any(scope["end"][3] == "selected" and int(scope["end"][2]) > 1 for scope in scopes))
        paired = [candidate for candidate in candidates if len(candidate["parameters"]) == 6]
        test.assertTrue(paired)
        test.assertTrue(any(candidate["parameters"][0][3] == "2" and candidate["parameters"][0][15].upper() == "SECOND"
                            and candidate["parameters"][3][3] == "1" for candidate in paired))


def check_nominal(test):
    source = test.source('''namespace First
namespace Inner
type Payload
    value as long
end type
end namespace
end namespace
namespace Second
namespace Inner
type Payload
    value as long
end type
end namespace
end namespace
declare sub UsePayload overload(byref value as First.Inner.Payload)
declare sub UsePayload overload(byref value as Second.Inner.Payload)
sub UsePayload(byref value as Second.Inner.Payload)
end sub
sub UsePayload(byref value as First.Inner.Payload)
end sub
''', "prototype-nominal.bas")
    model = test.compile(source)
    probes = [parameter for scope in model.prototype_scopes.values() for candidate in scope["candidates"]
              for parameter in candidate["parameters"] if parameter[4] == "subtype"]
    distinct = [probe for probe in probes if probe[5] == "1"]
    test.assertTrue(distinct)
    test.assertTrue(any(probe[31:33] == probe[39:41] and probe[33] != probe[41] for probe in distinct))
    test.assertTrue(any(probe[5] == "0" and probe[33] == probe[41] != "0" for probe in probes))


def check_methods(test):
    source = test.source('''type Container
    value as long
    declare property Item() as long
    declare property Item(byval value as long)
end type
property Container.Item() as long
    return this.value
end property
property Container.Item(byval value as long)
    this.value = value
end property
declare sub ArrayPick overload(values(any) as long)
declare sub ArrayPick overload(values(any, any) as long)
sub ArrayPick(values(any, any) as long)
end sub
sub ArrayPick(values(any) as long)
end sub
''', "prototype-methods.bas")
    model = test.compile(source)
    scopes = list(model.prototype_scopes.values())
    candidates = [candidate for scope in scopes for candidate in scope["candidates"]]
    test.assertTrue(any(candidate["event"][6] == "1" and candidate["event"][5] == "-1" for candidate in candidates))
    test.assertTrue(any(parameter[4:6] == ["bydesc", "1"] for candidate in candidates for parameter in candidate["parameters"]))
    test.assertTrue(any(scope["end"][3] == "selected" and int(scope["begin"][24]) > int(scope["candidates"][-1]["event"][3])
                        for scope in scopes if scope["candidates"]))


def check_modes(test):
    source = test.source(MATCHES, "prototype-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        for mode in ("full", "expressions"):
            model = test.compile(source, mode=mode, extra=extra)
            path = test.working / ("prototypes-" + mode + abi + ".fbsem")
            option = "-semantic-model" if mode == "full" else "-semantic-model-expressions"
            test.invoke([source], mode="off", extra=(*extra, option, str(path)))
            binary = Model.read(path, expressions_only=mode == "expressions")
            test.assertEqual(binary.prototype_events, model.prototype_events)
    model = test.compile(source, mode="bindings")
    test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(model.prototype_events)
    _, path = test.invoke([source, source])
    model = Model.read(path)
    test.assertEqual({module for module, identity in model.prototype_scopes}, {1, 2})


def check_rejection(test):
    source = test.source('''declare sub Work overload(byval value as long)
sub Work(byref value as long)
end sub
''', "prototype-mode-rejected.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
        result, model = recovered(test, source, backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        selected = [scope for scope in model.prototype_scopes.values() if scope["end"] is not None and scope["end"][3] == "selected"]
        test.assertTrue(selected)
        test.assertTrue(any(parameter[4:6] == ["bydesc", "0"] and parameter[6] != parameter[8]
                            for scope in selected for candidate in scope["candidates"] for parameter in candidate["parameters"]))


def check_corruption(test):
    model = test.compile(test.source(MATCHES, "prototype-corruption.bas"))
    for mutation in ("missing", "summary", "policy", "ordinal", "parent", "candidate", "stage", "remaining", "type", "bydesc", "formal", "symbol", "choice", "outcome", "duplicate"):
        with test.subTest(prototype_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            events = {name: next(row for row in rows if row[0] == "K" and row[3].startswith("native-prototype:")
                                and row[4].startswith(name + "\t")) for name in ("begin", "candidate", "parameter", "end")}
            summary = next(row for row in rows if row[3:4] == ["native-prototype-counts"])
            policy = next(row for row in rows if row[3:4] == ["native-prototype-policy"])
            if mutation in ("missing", "summary", "policy"):
                rows.remove({"missing": events["parameter"], "summary": summary, "policy": policy}[mutation])
            elif mutation == "ordinal":
                events["begin"][3] = "native-prototype:999999"
            elif mutation == "duplicate":
                rows.insert(-1, events["begin"].copy())
            else:
                name, index, value = {"parent": ("begin", 2, "999999"), "candidate": ("candidate", 2, "99"),
                                      "stage": ("parameter", 4, "guess"), "remaining": ("parameter", 3, "999"),
                                      "type": ("parameter", 12, "2147483648"), "bydesc": ("parameter", 5, "1"),
                                      "formal": ("parameter", 11, "1"), "symbol": ("parameter", 17, "999999"),
                                      "choice": ("end", 2, "999999"), "outcome": ("end", 3, "maybe")}[mutation]
                fields = events[name][4].split("\t")
                fields[index] = value
                events[name][4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of prototypes.py
