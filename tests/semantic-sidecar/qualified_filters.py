"""Project: FreeBASIC semantic sidecar tests
File: qualified_filters.py
Purpose: Check actual qualified search/filter joins and their exclusions.
Responsibilities: Native wrapper guards, explicit identities, transport and corruption.
This file intentionally does NOT join records by spelling or source positions.
"""

import subprocess

from driver_inputs import text_rows
from sidecar import Model
from semantic_qualified_filters import FEATURE

SOURCE = '''namespace Area
const Count = 3
type Payload
    value as long
end type
end namespace
dim value as Area.Payload
value.value = Area.Count
print value.value
print "repeat", "repeat"
goto Finished
Finished:
'''


def check_guards(test):
    native = (test.root / "src/compiler/symbols/symb.bas").read_text()
    bodies = []
    for name in ("symbLookupByNameAndClass", "symbFindByClass"):
        start = native.index("function " + name + " _")
        end = native.index("\nend function", start) + len("\nend function")
        bodies.append(native[start:end])
    guard = (test.root / "tests/semantic-sidecar/qualified-filter-guards.bas").read_text()
    index = guard.index("lex.ctx = @lex.ctxTB(0)")
    source = test.working / "qualified-filter-native-guards.bas"
    source.write_text(guard[:index] + "\n".join(bodies) + "\n\n" + guard[index:])
    executable = test.working / ("qualified-filter-guards.exe" if test.native_windows else "qualified-filter-guards")
    lookup_collector = test.working / "semantic-lookup.bas"
    lookup_collector.write_bytes((test.root / "src/compiler/tooling/semantic-lookup.bas").read_bytes())
    filter_collector = test.working / "semantic-filters.bas"
    filter_collector.write_bytes((test.root / "src/compiler/tooling/semantic-filters.bas").read_bytes())
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", source.stem, "-x", str(executable), str(source),
                             str(lookup_collector),
                             str(filter_collector)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("QUALIFIED_FILTER_GUARDS_PASS", result.stdout)


def check_links(test):
    source = test.source(SOURCE, "qualified-filter-links.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        links = model.qualified_filter_links[1]
        test.assertTrue(links)
        test.assertTrue(any(row[1] == "0" for row in links))
        test.assertTrue(any(row[1] != "0" and row[3] == "1" for row in links))
        for row in links:
            query = model.lookup_queries[1, int(row[0])]
            test.assertEqual(query["begin"][3], "qualified")
            if row[1] != "0":
                scope = model.filter_scopes[1, int(row[1])]
                test.assertEqual(scope["end"][4:], row[3:])


def check_modes(test):
    source = test.source(SOURCE, "qualified-filter-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        for mode in ("full", "expressions"):
            model = test.compile(source, mode=mode, extra=extra)
            path = test.working / ("qualified-filter-" + mode + abi + ".fbsem")
            option = "-semantic-model" if mode == "full" else "-semantic-model-expressions"
            test.invoke([source], mode="off", extra=(*extra, option, str(path)))
            binary = Model.read(path, expressions_only=mode == "expressions")
            test.assertEqual(binary.qualified_filter_links, model.qualified_filter_links)
    model = test.compile(source, mode="bindings")
    test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(model.qualified_filter_links)
    _, path = test.invoke([source, source])
    model = Model.read(path)
    test.assertEqual(set(model.qualified_filter_links), {1, 2})


def check_corruption(test):
    model = test.compile(test.source(SOURCE, "qualified-filter-corruption.bas"))
    for mutation in ("missing", "summary", "ordinal", "lookup", "filter", "class", "result", "duplicate"):
        with test.subTest(qualified_filter_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith("native-qualified-filter:"))
            summary = next(row for row in rows if row[3:4] == ["native-qualified-filter-count"])
            if mutation == "missing":
                rows.remove(row)
            elif mutation == "summary":
                rows.remove(summary)
            elif mutation == "ordinal":
                row[3] = "native-qualified-filter:999999"
            elif mutation == "duplicate":
                rows.insert(-1, row.copy())
            else:
                index, value = {"lookup": (0, "999999"), "filter": (1, "999999"), "class": (2, "0"), "result": (10, "999999")}[mutation]
                fields = row[4].split("\t")
                fields[index] = value
                row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of qualified_filters.py
