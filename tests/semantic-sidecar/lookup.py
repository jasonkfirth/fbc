"""Project: FreeBASIC semantic sidecar tests
File: lookup.py
Purpose: Exercise actual native name-search precedence and exclusions.
Responsibilities: Namespace/import/type/enum/keyword queries and returned-chain integrity.
This file intentionally does NOT implement lookup or infer executed calls.
"""

import subprocess

from driver_inputs import text_rows
from recovery_steps import recovered
from semantic_lookup import FEATURE
from sidecar import Model


def queries(model, spelling):
    return [query for query in model.lookup_queries.values() if query["begin"][4].upper() == spelling.upper()]


def reasons(query):
    return {step[3] for step in query["steps"] if step[0] == "decision"}


def check_guards(test):
    executable = test.working / ("lookup-guards.exe" if test.native_windows else "lookup-guards")
    sources = [test.root / "tests/semantic-sidecar/lookup-guards.bas",
               test.root / "src/compiler/tooling/semantic-lookup.bas"]
    copied_sources = []
    for source in sources:
        local = test.working / source.name
        local.write_bytes(source.read_bytes())
        copied_sources.append(local)
    sources = copied_sources
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", "lookup-guards", "-x", str(executable), *map(str, sources)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("LOOKUP_GUARDS_PASS", result.stdout)


def check_precedence(test):
    cases = (
        ("own", '''namespace Library
dim shared Item as long
end namespace
namespace LocalArea
using Library
dim shared Item as long
sub UseItem()
print Item
end sub
end namespace
print Library.Item
''', "Item", {"lexical-selected"}),
        ("imports", '''namespace First
dim shared Imported as long
end namespace
namespace Second
using First
sub UseImported()
print Imported
end sub
end namespace
print First.Imported
''', "Imported", {"imports-returned"}),
        ("types", '''type BaseValue
    member as long
    enum Elements
        InheritedElement = 1
    end enum
end type
type DerivedValue extends BaseValue
    declare sub ReadMember()
end type
sub DerivedValue.ReadMember()
    print member, InheritedElement
end sub
dim value as DerivedValue
print value.member
''', "member", {"base-member-selected"}),
        ("enum", '''dim shared ElementName as long
namespace Enclosing
enum Values
    ElementName = 3
end enum
sub UseElement()
    print ElementName
end sub
end namespace
''', "ElementName", {"active-enum-selected"}),
    )
    for backend in test.backends:
        for label, text, spelling, expected in cases:
            with test.subTest(backend=backend, lookup=label):
                source = test.source(text, "lookup-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend)
                emitted = test.emission_path(source.stem, backend).read_bytes()
                result, path = test.invoke([source], backend=backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emitted)
                model = Model.read(path)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                matches = queries(model, spelling)
                test.assertTrue(matches)
                actual = set().union(*(reasons(query) for query in matches))
                test.assertFalse(expected - actual, (expected, actual))
                test.assertTrue(all(query["end"] is not None for query in model.lookup_queries.values()))
                test.assertTrue(any(query["begin"][3] == "qualified" for query in model.lookup_queries.values()))


def check_ambiguity(test):
    source = test.source('''namespace First
dim shared Duplicate as long
end namespace
namespace Second
dim shared Duplicate as long
end namespace
using First, Second
print Duplicate
''', "lookup-ambiguous.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
        result, model = recovered(test, source, backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        matches = [query for query in queries(model, "Duplicate") if len(query["results"]) > 1]
        test.assertTrue(matches)
        test.assertTrue(all(item[3] == "1" for query in matches for item in query["results"]))
        test.assertFalse(any(int(event[7]) for event in model.resolution_events.get(1, ()) if event[0] == "end"))


def check_keywords(test):
    source = test.source('''#define USTRING string
dim value as USTRING
print value
#undef USTRING
dim unicodeValue as ustring
''', "lookup-keyword-shadow.bas")
    model = test.compile(source)
    test.assertTrue(any(query["end"][5] == "macro-shadow" for query in queries(model, "USTRING")))
    test.assertTrue(any(query["end"][5] == "keyword" for query in queries(model, "DIM")))
    test.assertTrue(any("defer-quirk-keyword" in reasons(query) for query in queries(model, "PRINT")))
    source = test.source('#lang "qb"\ndim width%\nwidth% = 1\n', "lookup-quirk.bas")
    model = test.compile(source)
    test.assertTrue(any("defer-quirk-keyword" in reasons(query) for query in queries(model, "WIDTH")))
    source = test.source('#define FB_NO_USTRING\ndim ustring as long\nustring = 1\n', "lookup-disabled-keyword.bas")
    model = test.compile(source)
    test.assertTrue(any("keyword-disabled" in reasons(query) for query in queries(model, "USTRING")))
    test.assertTrue(any(item[5] == "1" for query in queries(model, "USTRING") for item in query["results"]))


def check_lifetimes(test):
    source = test.source('''namespace Box
type Pending as Later
dim shared pointerValue as Pending ptr
type Later
    value as long
end type
end namespace
#define NAME_ALIAS Box.Later
dim beforeType as NAME_ALIAS
#undef NAME_ALIAS
#define NAME_ALIAS long
dim afterType as NAME_ALIAS
#undef NAME_ALIAS
print beforeType.value, afterType
''', "lookup-lifetimes.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        later = queries(model, "Later")
        test.assertTrue(any(not query["results"] for query in later))
        test.assertTrue(any(query["results"] for query in later))
        macros = {int(item[11]) for query in queries(model, "NAME_ALIAS")
                  for item in query["results"] if item[5] == "5"}
        test.assertEqual(len(macros), 2)
        test.assertTrue(all(identity in model.symbols for identity in macros))


def check_modes(test):
    source = test.source('namespace Area\ndim shared Known as double\nend namespace\nprint Area.Known\n', "lookup-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        for mode in ("full", "expressions"):
            model = test.compile(source, mode=mode, extra=extra)
            datatypes = {int(item[6]) for query in queries(model, "Known") for item in query["results"]}
            test.assertEqual(datatypes, {18 if abi == "native" else 16})
            path = test.working / ("lookup-" + mode + abi + ".fbsem")
            option = "-semantic-model" if mode == "full" else "-semantic-model-expressions"
            test.invoke([source], mode="off", extra=(*extra, option, str(path)))
            binary = Model.read(path, expressions_only=mode == "expressions")
            test.assertEqual(binary.lookup_events, model.lookup_events)
            if mode == "expressions":
                test.assertTrue(all(item[11] == "0" for query in model.lookup_queries.values() for item in query["results"]))
    binding = test.compile(source, mode="bindings")
    test.assertEqual(binding.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(binding.lookup_events)
    _, path = test.invoke([source, source])
    model = Model.read(path)
    test.assertEqual({module for module, identity in model.lookup_queries}, {1, 2})


def check_corruption(test):
    model = test.compile(test.source('namespace Area\ndim shared Known as long\nend namespace\nprint Area.Known\n', "lookup-corruption.bas"))
    for mutation in ("missing", "summary", "count", "ordinal", "parent", "route", "probe", "class", "symbol", "result", "result-count", "token", "duplicate", "direct-import", "keyword-return", "summary-order"):
        with test.subTest(lookup_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            events = {name: next(row for row in rows if row[3:4] and row[3].startswith("native-lookup:")
                                and row[4].startswith(name + "\t")) for name in ("begin", "probe", "result", "end")}
            summary = next(row for row in rows if row[3:4] == ["native-lookup-counts"])
            if mutation in ("missing", "summary"):
                rows.remove(events["probe"] if mutation == "missing" else summary)
            elif mutation == "count":
                summary[4] = "0\t0\t0\t0\t0\t0\t0"
            elif mutation == "ordinal":
                events["begin"][3] = "native-lookup:999999"
            elif mutation == "duplicate":
                rows.insert(-1, events["begin"].copy())
            elif mutation == "summary-order":
                rows.remove(summary)
                rows.insert(rows.index(events["begin"]), summary)
            elif mutation in ("direct-import", "keyword-return"):
                ending = next(row[4].split("\t") for row in rows if row[0] == "K"
                              and row[3].startswith("native-lookup:")
                              and row[4].startswith("end\t") and row[4].endswith("\tkeyword"))
                result = next(row for row in rows if row[0] == "K" and row[3].startswith("native-lookup:")
                              and row[4].startswith("result\t" + ending[1] + "\t"))
                fields = result[4].split("\t")
                if mutation == "direct-import":
                    fields[3] = "1"
                else:
                    fields[5] = "1"
                    fields[11] = str(next(identity for identity, symbol in model.symbols.items() if symbol[3] == "1"))
                result[4] = "\t".join(fields)
            else:
                name, index, value = {"parent": ("begin", 2, "999999"), "route": ("begin", 3, "hypothetical"),
                                      "probe": ("probe", 3, "guess"), "class": ("result", 5, "99"),
                                      "symbol": ("result", 11, "999999"), "result": ("result", 2, "2"),
                                      "result-count": ("end", 4, "999999"), "token": ("end", 3, "99")}[mutation]
                fields = events[name][4].split("\t"); fields[index] = value; events[name][4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of lookup.py
