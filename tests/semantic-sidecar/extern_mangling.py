"""Project: FreeBASIC semantic sidecar tests
File: extern_mangling.py
Purpose: Prove actual EXTERN policy selection and restoration are retained.
Responsibilities: Native grammar branches, declaration contexts, recovery and exclusions.
This file intentionally does NOT infer final external spellings from parser policy.
"""

import subprocess

from driver_inputs import text_rows
from semantic_contexts import EXTERN_OPTION
from semantic_extern import FEATURE, PREFIX
from sidecar import Model

SOURCE = '''extern "C"
declare sub CProcedure()
extern "C++"
namespace Namespaced
declare sub CppProcedure()
end namespace
extern "c++"
declare sub RepeatedProcedure()
end extern
end extern
declare sub RestoredProcedure()
end extern
extern "Windows"
declare sub WindowsProcedure()
end extern
extern "windows-ms"
declare sub MsProcedure()
end extern
extern "rtlib"
declare sub RtlProcedure()
end extern
dim value as long = 7
print value
'''


def check_guards(test):
    sources = []
    for relative in ("tests/semantic-sidecar/extern-mangling-guards.bas", "src/compiler/tooling/semantic-extern.bas"):
        source = test.working / relative.rsplit("/", 1)[1]
        source.write_bytes((test.root / relative).read_bytes())
        sources.append(source)
    executable = test.working / ("extern-mangling-guards.exe" if test.native_windows else "extern-mangling-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", sources[0].stem, "-x", str(executable), *map(str, sources)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=300)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("EXTERN_MANGLING_GUARDS_PASS", result.stdout)


def check_scopes(test):
    source = test.source(SOURCE, "extern-scopes.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        transitions = model.extern_transitions[1]
        test.assertEqual(len(transitions), 12)
        begins = [row for row in transitions if row[0] == "begin"]
        test.assertEqual([row[10] for row in begins], ["C", "C++", "c++", "Windows", "windows-ms", "rtlib"])
        test.assertEqual([int(row[3]) for row in begins], [1, 4, 4, 2, 3, 6])
        test.assertTrue(all(row[8:10] == ["1", "0"] for row in begins))
        # Repeating the same mode is still a distinct native scope, even
        # though the effective snapshot does not need a new identity.
        repeated = begins[2]
        test.assertEqual(repeated[4], repeated[5])
        for name, mode in (("CProcedure", 1), ("CppProcedure", 4), ("RepeatedProcedure", 4),
                           ("RestoredProcedure", 1), ("WindowsProcedure", 2), ("MsProcedure", 3),
                           ("RtlProcedure", 6)):
            symbols = set(model.named(name))
            test.assertTrue(symbols, name)
            declarations = [row for row in model.records["DCL"] if int(row[2]) in symbols]
            test.assertTrue(declarations, name)
            for declaration in declarations:
                context = model.context_uses["declaration", int(declaration[1])]
                test.assertEqual(model.options[context][EXTERN_OPTION], mode, name)
        # Ordinary variables use their actual B declaration and OWN/ST
        # context, rather than the special DCL receipt used by prototypes.
        variables = set(model.named("value"))
        bindings = [ordinal for ordinal, row in enumerate(model.records["B"], 1)
                    if int(row[1]) in variables and row[2] == "declaration"]
        test.assertTrue(bindings)
        for binding in bindings:
            statement = model.statement_owners["binding", binding]
            context = int(model.statements[statement][6])
            test.assertEqual(model.options[context][EXTERN_OPTION], 0)


def check_modes(test):
    source = test.source(SOURCE, "extern-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        model = test.compile(source, extra=extra)
        binary_path = test.working / ("extern-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(binary_path)))
        test.assertEqual(Model.read(binary_path).extern_transitions, model.extern_transitions)
    expressions = test.compile(source, mode="expressions")
    test.assertEqual(expressions.capabilities[1][FEATURE], "available")
    test.assertFalse(expressions.configurations)
    test.assertEqual(len(expressions.extern_transitions[1]), 12)
    test.assertTrue(all(row[4:6] == ["0", "0"] for row in expressions.extern_transitions[1]))
    bindings = test.compile(source, mode="bindings")
    test.assertEqual(bindings.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(bindings.extern_transitions)
    empty = test.source("print 1\n", "extern-no-scopes.bas")
    _, path = test.invoke([source, empty, source])
    model = Model.read(path)
    test.assertEqual([len(model.extern_transitions[module]) for module in (1, 2, 3)], [12, 0, 12])
    test.assertTrue(all(model.options[identity][EXTERN_OPTION] == 0 for identity, module in model.configurations.items() if module == 2))


def check_variable_declarations(test):
    source = test.source('''extern "C"
extern CVariable as long
extern "C++"
extern CppVariable as long
end extern
extern RestoredVariable as long
end extern
dim LocalVariable as long
print LocalVariable
''', "extern-variable-declarations.bas")
    for backend in test.backends:
        model = test.compile(source, backend=backend)
        test.assertEqual(len(model.extern_transitions[1]), 4)
        for name, mode in (("CVariable", 1), ("CppVariable", 4), ("RestoredVariable", 1), ("LocalVariable", 0)):
            symbols = set(model.named(name))
            bindings = [ordinal for ordinal, row in enumerate(model.records["B"], 1)
                        if int(row[1]) in symbols and row[2] == "declaration"]
            test.assertTrue(bindings, name)
            for binding in bindings:
                statement = model.statement_owners["binding", binding]
                context = int(model.statements[statement][6])
                test.assertEqual(model.options[context][EXTERN_OPTION], mode, name)


def check_recovery(test):
    cases = (("unknown", 'extern "not-a-mode"\ndeclare sub RecoveredProc()\nend extern\n', "not-a-mode"),
             ("missing", 'extern\ndeclare sub RecoveredProc()\nend extern\n', ""),
             ("unterminated", 'extern "C++"\ndeclare sub UnclosedProc()\n', None),
             ("orphan", 'end extern\n', None))
    for backend in test.backends:
        for label, text, spelling in cases:
            with test.subTest(backend=backend, case=label):
                source = test.source(text, "extern-recovery-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, path = test.invoke([source], mode="expressions", backend=backend, success=False)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                model = Model.read(path, expressions_only=True, allow_recovery=True)
                test.assertEqual(model.recovered_modules, {1})
                transitions = model.extern_transitions[1]
                if spelling is not None and label != "missing":
                    test.assertEqual(len(transitions), 2)
                    test.assertEqual(transitions[0][3], "1")
                    test.assertEqual(transitions[0][8:11], [str(int(bool(spelling))), "1", spelling])
                    test.assertEqual(transitions[1][3], "0")
                elif label == "unterminated":
                    test.assertEqual(len(transitions), 1)
                    test.assertTrue(all(scope["end"] is None for scope in model.extern_scopes.values()))
                else:
                    # Bare EXTERN is routed through variable declaration
                    # recovery before the compound parser is entered.
                    test.assertFalse(transitions)
                _, full_path = test.invoke([source], backend=backend, success=False)
                if full_path.exists():
                    # Failed full output can retain its header/dependency
                    # prefix. It has no complete footer or committed module.
                    with test.assertRaisesRegex(ValueError, "completeness footer"):
                        Model.read(full_path)
                    lines = full_path.read_text().splitlines()
                    test.assertFalse(any(line.startswith(("BLK\t", "CTX\t", "END\t")) or "extern-mangling" in line for line in lines))


def check_origins(test):
    include = test.source('extern "C"\ndeclare sub HeaderProcedure()\nend extern\n', "extern-header.bi")
    source = test.source('''#define BEGIN_POLICY extern "C++"
BEGIN_POLICY
#include "extern-header.bi"
declare sub OuterProcedure()
end extern
#if 0
extern "not-a-mode"
declare sub InactiveProcedure()
end extern
#endif
#line 90 "logical-extern.bas"
extern "windows"
declare sub RemappedProcedure()
end extern
''', "extern-origins.bas")
    model = test.compile(source)
    begins = [row for row in model.extern_transitions[1] if row[0] == "begin"]
    test.assertEqual(len(begins), 3)
    test.assertGreater(int(begins[0][17]), 0)
    test.assertEqual(begins[0][16], "0")
    test.assertEqual(model.source_contexts[int(begins[1][7])][5], "include")
    test.assertEqual(begins[1][11], str(include))
    test.assertEqual(begins[2][11], "logical-extern.bas")
    test.assertEqual(begins[2][16], "0")
    test.assertFalse(model.named("InactiveProcedure"))
    for mode in ("full", "expressions"):
        hidden = test.compile(source, mode=mode, extra=("-semantic-model-compact",))
        test.assertFalse(hidden.macro_invocations)
        test.assertTrue(hidden.extern_transitions[1])
        test.assertTrue(all(row[17:20] == ["0", "0", "0"] for row in hidden.extern_transitions[1]))
    _, path = test.invoke([source, source])
    repeated = Model.read(path)
    rows = [row.copy() for row in repeated.rows]
    event = next(row for row in rows if row[0] == "K" and row[2] == "1" and row[3].startswith(PREFIX + ":")
                 and row[4].split("\t")[17] != "0")
    fields = event[4].split("\t")
    foreign = next(row[17] for row in repeated.extern_transitions[2] if row[17] != "0")
    fields[17] = foreign
    event[4] = "\t".join(fields)
    with test.assertRaises(ValueError):
        Model(text_rows(rows))
    # QB rejects the compound before native scope allocation. Its skipped
    # body must not receive a fabricated EXTERN selection or declaration.
    rejected = test.source('#lang "qb"\nextern "C"\ndeclare sub SkippedProcedure()\nend extern\n', "extern-qb.bas")
    _, path = test.invoke([rejected], mode="expressions", success=False)
    model = Model.read(path, expressions_only=True, allow_recovery=True)
    test.assertFalse(model.extern_transitions[1])


def check_corruption(test):
    model = test.compile(test.source(SOURCE, "extern-corruption.bas"))
    mutations = ("missing-event", "missing-counts", "count", "ordinal", "duplicate", "kind", "construct", "mode", "before-context", "after-context",
                 "statement", "source", "present", "recovered", "spelling", "physical", "macro", "macro-offset", "option", "missing-option", "removed-all", "restoration")
    for mutation in mutations:
        with test.subTest(extern_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            event = next(row for row in rows if row[3:4] and row[3].startswith(PREFIX + ":"))
            summary = next(row for row in rows if row[3:4] == [PREFIX + "-counts"])
            if mutation in ("missing-event", "missing-counts"):
                rows.remove(event if mutation == "missing-event" else summary)
            elif mutation == "removed-all":
                rows = [row for row in rows if not (row[0] == "K" and row[3].startswith(PREFIX + ":"))]
                summary[4] = "0\t0\t0\t0"
            elif mutation == "count":
                summary[4] = "0\t0\t0\t0"
            elif mutation == "ordinal":
                event[3] = PREFIX + ":999999"
            elif mutation == "duplicate":
                rows.insert(-1, event.copy())
            elif mutation == "restoration":
                opening = event[4].split("\t")
                ending = next(row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":")
                              and row[4].split("\t")[:2] == ["end", opening[1]])
                fields = ending[4].split("\t")
                fields[3], fields[5] = opening[3], opening[5]
                ending[4] = "\t".join(fields)
            elif mutation in ("option", "missing-option"):
                option = next(row for row in rows if row[0] == "OPT" and row[2:4] == list(EXTERN_OPTION))
                if mutation == "missing-option":
                    rows.remove(option)
                else:
                    option[4] = "99"
            else:
                index, value = {"kind": (0, "guessed"), "construct": (1, "999999"), "mode": (3, "5"),
                                "before-context": (4, "999999"), "after-context": (5, "999999"), "statement": (6, "999999"),
                                "source": (7, "999999"), "present": (8, "0"), "recovered": (9, "1"),
                                "spelling": (10, "guessed"), "physical": (12, "0"), "macro": (17, "999999"),
                                "macro-offset": (18, "1")}[mutation]
                fields = event[4].split("\t")
                fields[index] = value
                event[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_runtime(test):
    source = test.source(SOURCE, "extern-runtime.bas")
    for backend in test.backends:
        outputs = []
        for mode in ("off", "full", "expressions", "bindings"):
            executable = test.working / ("extern-" + backend + "-" + mode + (".exe" if test.native_windows else ""))
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            outputs.append(result.stdout)
        test.assertEqual(outputs, [" 7\n"] * 4)

# end of extern_mangling.py
