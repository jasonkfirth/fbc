"""Project: FreeBASIC semantic sidecar tests
File: filters.py
Purpose: Check actual native symbol-filter classes and their exclusions.
Responsibilities: Production helper guards, source fixtures, transport and corruption.
This file intentionally does NOT provide alternate lookup or filter algorithms.
"""

import subprocess

from driver_inputs import text_rows
from recovery_steps import recovered
from semantic_filters import FEATURE
from sidecar import Model


def check_guards(test):
    # Compile the real four filter bodies with controlled symbols and a
    # counting native access hook. This exposes stop-after-denial behavior
    # that cannot be inferred from a successfully bound source reference.
    native = (test.root / "src/compiler/symbols/symb.bas").read_text()
    bodies = []
    for name in ("symbFindByClass", "symbFindByClassAndType", "symbFindVarBySuffix", "symbFindVarByDefType"):
        start = native.index("function " + name + " _")
        end = native.index("\nend function", start) + len("\nend function")
        bodies.append(native[start:end])
    source = test.working / "filters-native-guards.bas"
    guard = (test.root / "tests/semantic-sidecar/filters-guards.bas").read_text()
    marker = "lex.ctx = @lex.ctxTB(0)"
    index = guard.index(marker)
    source.write_text(guard[:index] + "\n".join(bodies) + "\n\n" + guard[index:])
    executable = test.working / ("filters-guards.exe" if test.native_windows else "filters-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", source.stem, "-x", str(executable), str(source),
                             str(test.root / "src/compiler/tooling/semantic-filters.bas")],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("FILTER_GUARDS_PASS", result.stdout)


def check_classes(test):
    cases = (
        ("typed", '''namespace Area
const Named = 3
extern value as double
dim shared value as double
end namespace
print Area.Named, Area.value
'''),
        ("qb", '''#lang "qb"
defint a-z
dim item%, item$, plain as double
item% = 3
item$ = "text"
plain = 4
defstr s
dim sValue$
sValue$ = "string"
print item%, item$, item, plain, sValue
'''),
        ("fblite", '''#lang "fblite"
defstr s
sValue = "text"
dim numberValue as double
numberValue = 4
print sValue, numberValue
'''),
        ("redeclarations", '''#lang "fblite"
defint a-z
dim item%, item$, item!
defdbl i
dim item
defstr s
dim sArray$()
redim sArray$(0 to 1)
redim preserve sArray(0 to 2)
print item, sArray(0)
'''),
    )
    kinds = set()
    reasons = set()
    for backend in test.backends:
        for label, text in cases:
            with test.subTest(backend=backend, native_filter=label):
                source = test.source(text, "filters-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend)
                emission = test.emission_path(source.stem, backend).read_bytes()
                result, path = test.invoke([source], backend=backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
                model = Model.read(path)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                kinds.update(scope["begin"][3] for scope in model.filter_scopes.values())
                reasons.update(candidate[4] for scope in model.filter_scopes.values() for candidate in scope["candidates"])
    test.assertEqual(kinds, {"class", "class-and-type", "suffix", "default-type"})
    test.assertTrue({"class", "class-and-type", "suffix-string", "suffix-type", "default-string",
                     "default-type", "default-unsuffixed"} <= reasons, reasons)


def check_modes(test):
    source = test.source('dim value as double\nvalue = 3\nprint value\n', "filters-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        for mode in ("full", "expressions"):
            model = test.compile(source, mode=mode, extra=extra)
            path = test.working / ("filters-" + mode + abi + ".fbsem")
            option = "-semantic-model" if mode == "full" else "-semantic-model-expressions"
            test.invoke([source], mode="off", extra=(*extra, option, str(path)))
            binary = Model.read(path, expressions_only=mode == "expressions")
            test.assertEqual(binary.filter_events, model.filter_events)
    model = test.compile(source, mode="bindings")
    test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(model.filter_events)
    _, path = test.invoke([source, source])
    model = Model.read(path)
    test.assertEqual({module for module, identity in model.filter_scopes}, {1, 2})


def check_rejections(test):
    cases = (
        ("access", 'dim hidden as long\nsub UseHidden()\nprint hidden\nend sub\n', "denied"),
        ("type", 'extern item as long\ndim shared item as double\n', "not-found"),
        ("suffix", '#lang "fblite"\noption explicit\ndim item as long\nprint item$\n', "not-found"),
    )
    for backend in test.backends:
        for label, source_text, outcome in cases:
            with test.subTest(backend=backend, rejected_filter=label):
                source = test.source(source_text, "filters-rejected-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, model = recovered(test, source, backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                scopes = [scope for scope in model.filter_scopes.values()
                          if scope["end"] is not None and scope["end"][3] == outcome]
                test.assertTrue(scopes)
                test.assertTrue(all(scope["end"][2] == "0" and scope["end"][4] == "0" for scope in scopes))


def check_corruption(test):
    model = test.compile(test.source('dim value as double\nvalue = 3\nprint value\n', "filters-corruption.bas"))
    for mutation in ("missing", "summary", "policy", "ordinal", "parent", "kind", "predicate", "candidate", "symbol", "access", "choice", "outcome", "duplicate"):
        with test.subTest(filter_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            events = {name: next(row for row in rows if row[0] == "K" and row[3].startswith("native-symbol-filter:")
                                and row[4].startswith(name + "\t")) for name in ("begin", "candidate", "access", "end")}
            summary = next(row for row in rows if row[3:4] == ["native-symbol-filter-counts"])
            policy = next(row for row in rows if row[3:4] == ["native-symbol-filter-policy"])
            if mutation in ("missing", "summary", "policy"):
                rows.remove({"missing": events["candidate"], "summary": summary, "policy": policy}[mutation])
            elif mutation == "ordinal":
                events["begin"][3] = "native-symbol-filter:999999"
            elif mutation == "duplicate":
                rows.insert(-1, events["begin"].copy())
            else:
                name, index, value = {"parent": ("begin", 2, "999999"), "kind": ("begin", 3, "hypothetical"),
                                      "predicate": ("candidate", 4, "guess"), "candidate": ("candidate", 2, "99"),
                                      "symbol": ("candidate", 12, "999999"), "access": ("access", 3, "9"),
                                      "choice": ("end", 2, "999999"), "outcome": ("end", 3, "maybe")}[mutation]
                fields = events[name][4].split("\t")
                fields[index] = value
                events[name][4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of filters.py
