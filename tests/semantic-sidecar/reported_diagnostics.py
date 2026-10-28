"""Project: FreeBASIC semantic sidecar tests
File: reported_diagnostics.py
Purpose: Prove structured observations of accepted native diagnostics.
Responsibilities: Categories, policy, parameters, suppression and both artifacts.
This file intentionally does NOT infer semantic facts by parsing console output.
"""

import subprocess

from diagnostic_context import FEATURE
from diagnostics import Diagnostics
from sidecar import Model


def observe(test, source, *, success=False, extra=(), mode="off", backend="gcc"):
    path = test.working / ("reported-" + str(test.sequence + 1) + ".fbcdia")
    result, model_path = test.invoke([source], mode=mode, success=success, backend=backend,
                                     extra=(*extra, "-semantic-diagnostics", str(path)))
    return result, Diagnostics.read(path, compiler_exit=result.returncode), model_path


def check_categories(test):
    cases = {
        "syntax": "dim value as\n",
        "lookup": "print UnknownReportedValue\n",
        "parameter": 'declare sub ParameterCall(byref value as long)\nParameterCall("wrong")\n',
        "overload": 'declare sub Choice overload(byref value as long)\ndeclare sub Choice overload(byref value as double)\nChoice("wrong")\n',
        "access": "type Restricted\nprivate:\nvalue as long\nend type\ndim instance as Restricted\nprint instance.value\n",
        "layout": "type BrokenLayout\nmember as string * -1\nend type\n",
        "include": '#include "missing-diagnostic-include.bi"\n',
        "limit": "@\n@\n@\n",
    }
    for name, text in cases.items():
        with test.subTest(category=name):
            source = test.source(text, "reported-" + name + ".bas")
            extra = ("-maxerr", "1") if name == "limit" else ()
            baseline, _ = test.invoke([source], mode="off", success=False, extra=extra)
            result, diagnostic, _ = observe(test, source, extra=extra)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            test.assertTrue(diagnostic.records)
            test.assertTrue(diagnostic.reported_contexts)
            if name == "layout":
                test.assertIn("310", [row[4] for row in diagnostic.records])
            if name == "access":
                test.assertIn("202", [row[4] for row in diagnostic.records])
            test.assertEqual(set(diagnostic.contexts), set(range(1, len(diagnostic.records) + 1)))
            for identity, context in diagnostic.contexts.items():
                test.assertEqual(context[0], "1")
                test.assertEqual(int(context[7]), len(diagnostic.parameters.get(identity, ())))
            if name == "parameter":
                frames = [frame for values in diagnostic.parameters.values() for frame in values]
                test.assertTrue(any(frame[2] == "1" and frame[4].casefold() == "parametercall" for frame in frames))


def check_warnings(test):
    source = test.source("function ReportedWarning(value as long) as long\nend function\n", "reported-warning.bas")
    baseline, _ = test.invoke([source], mode="off", extra=("-w", "all"))
    result, diagnostic, _ = observe(test, source, success=True, extra=("-w", "all"))
    test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
    test.assertTrue(diagnostic.records)
    test.assertTrue(all(row[3] == "warning" for row in diagnostic.records))
    test.assertTrue(all(context[6] == "0" for context in diagnostic.contexts.values()))
    _, hidden, _ = observe(test, source, success=True, extra=("-w", "none"))
    test.assertFalse(hidden.records)
    test.assertFalse(hidden.contexts)
    result, promoted, _ = observe(test, source, extra=("-w", "all", "-w", "error"))
    test.assertTrue(promoted.records)
    test.assertTrue(any(context[6] == "1" for context in promoted.contexts.values()))
    test.assertFalse(promoted.succeeded)


def check_sidecar(test):
    warning = test.source("function ContextWarning() as long\nend function\n", "sidecar-diagnostic-warning.bas")
    for mode in ("full", "expressions", "bindings"):
        with test.subTest(mode=mode):
            _, diagnostic, path = observe(test, warning, success=True, mode=mode, extra=("-w", "all"))
            model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertEqual(len(model.diagnostics), len(diagnostic.records))
            test.assertEqual(list(model.diagnostic_contexts.values()), list(diagnostic.contexts.values()))
    invalid = test.source('declare sub ContextParameter(byref value as long)\nContextParameter("wrong")\n', "sidecar-diagnostic-error.bas")
    _, diagnostic, path = observe(test, invalid, mode="expressions")
    model = Model.read(path, expressions_only=True, allow_recovery=True)
    test.assertEqual(model.footer[0], "RECOVERY")
    test.assertTrue(model.diagnostic_parameters)
    test.assertEqual(list(model.diagnostic_parameters.values()), list(diagnostic.parameters.values()))


def check_rejection(test):
    source = test.source('declare sub ContextParameter(byref value as long)\nContextParameter("wrong")\n', "mutated-diagnostic-context.bas")
    observe(test, source)
    original = next(path for path in test.working.glob("reported-*.fbcdia"))
    from sidecar import unescape
    rows = [[unescape(field) for field in line.split("\t")] for line in original.read_text().splitlines()]
    def encoded(items):
        def escape(value):
            return "".join(chr(byte) if 32 <= byte < 127 and byte != 37 else f"%{byte:02X}"
                           for byte in value.encode("utf-8", errors="surrogateescape"))
        return "\n".join("\t".join(escape(field) for field in row) for row in items) + "\n"
    for mutation in ("missing-context", "parameter-count", "policy-flag", "wrong-parameter-owner", "parameter-order", "kind", "invocation-source", "missing-capability"):
        with test.subTest(mutation=mutation):
            changed = [row.copy() for row in rows]
            context = next(row for row in changed if row[0] == "DC")
            frame = next(row for row in changed if row[0] == "DP")
            if mutation == "missing-context":
                changed.remove(context)
            elif mutation in ("parameter-count", "policy-flag"):
                fields = context[2].split("\t")
                field = 7 if mutation == "parameter-count" else 0
                fields[field] = "0" if mutation == "parameter-count" else "2"
                context[2] = "\t".join(fields)
            elif mutation == "wrong-parameter-owner":
                frame[1] = "100000"
            elif mutation == "parameter-order":
                frame[2] = "2"
            elif mutation == "kind":
                frame[8] = "guaranteed-executed"
            elif mutation == "invocation-source":
                next(row for row in changed if row[0] == "DI")[2] = "0"
            else:
                changed = [row for row in changed if not (row[0] == "CAP" and row[1] == "0")]
            path = test.working / ("mutation-" + mutation + ".fbcdia")
            path.write_text(encoded(changed))
            with test.assertRaises(ValueError):
                Diagnostics.read(path)


def check_native_reporter(test):
    executable = test.working / ("reporter-guards.exe" if test.native_windows else "reporter-guards")
    sources = [test.root / "tests/semantic-sidecar/reported-diagnostic-guards.bas"]
    sources += [test.root / "src/compiler" / unit for unit in (
        "diagnostics/error.bas", "support/containers/list.bas",
        "support/containers/hash.bas", "support/allocate.bas")]
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
               "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
               "-m", "reported-diagnostic-guards", "-x", str(executable), *map(str, sources)]
    result = subprocess.run(command, cwd=test.working, text=True, capture_output=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, text=True, capture_output=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("REPORTED_DIAGNOSTIC_GUARDS_PASS", result.stdout)


def check_attempts(test):
    preinclude = test.source("function BeforeRestart() as long\nend function\n", "reported-preinclude.bi")
    source = test.source('#lang "fblite"\nprint 1\n', "reported-restart.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            extra = ("-include", str(preinclude))
            baseline, _ = test.invoke([source], mode="off", backend=backend, extra=extra)
            result, diagnostic, _ = observe(test, source, success=True, backend=backend, extra=extra)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            test.assertEqual(diagnostic.attempts, {1: ("restarted", 0), 2: ("accepted", 0)})
            test.assertEqual(len(diagnostic.records), 2)
            test.assertEqual(len(diagnostic.current_records), 1)
            test.assertEqual(diagnostic.current_records[0][2], "2")
    bad = test.source("print MissingRestartValue\n", "rejected-attempt.bas")
    _, diagnostic, _ = observe(test, bad)
    test.assertEqual(diagnostic.attempts, {1: ("rejected", 1)})
    test.assertEqual(diagnostic.current_records, diagnostic.records)


def check_backend_and_invocation(test):
    source = test.source("print 1\n", "reported-backend.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            stem = test.working / ("blocked-emission-" + backend)
            emitted = stem.with_suffix(".ll" if backend == "llvm" else ".c")
            emitted.mkdir()
            (emitted / "occupied").write_text("keep the output directory nonempty")
            extra = ("-o", str(stem.with_suffix(".o")))
            baseline, _ = test.invoke([source], mode="off", backend=backend, success=False, extra=extra)
            result, diagnostic, _ = observe(test, source, backend=backend, extra=extra)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            test.assertTrue(diagnostic.records)
            test.assertEqual(diagnostic.attempts, {1: ("rejected", 1)})
            test.assertTrue(all(row[3] == "error" for row in diagnostic.records))
    # A real QB object selects the native mixed-language warning after parser
    # teardown. Its object identity and native code prove the reporting route.
    old = test.source('#lang "qb"\nsub qbhelper\nend sub\n', "reported-old-language.bas")
    object_file = test.working / "reported-old-language.o"
    test.invoke([old], mode="off", emit=False, extra=("-c", "-o", str(object_file)))
    output = test.working / ("mixed.exe" if test.native_windows else "mixed")
    artifact = test.working / "invocation.fbcdia"
    result, _ = test.invoke([source], mode="off", emit=False,
                           extra=("-w", "all", "-semantic-diagnostics", str(artifact),
                                  str(object_file), "-x", str(output)))
    diagnostic = Diagnostics.read(artifact, compiler_exit=result.returncode)
    invocation = [row for row in diagnostic.records if row[2] == "0"]
    test.assertTrue(any(row[3:5] == ["warning", "21"] for row in invocation))
    for row in invocation:
        test.assertEqual(row[5:7], ["", ""])
        test.assertEqual(row[11:], ["0", "0", "0"])
        test.assertEqual(diagnostic.contexts[int(row[1])][0], "0")
        test.assertFalse(diagnostic.parameters.get(int(row[1])))


def check_transport_and_rejection(test):
    from driver_inputs import text_rows
    source = test.source("function ModelWarning() as long\nend function\n", "diagnostic-transport.bas")
    text = test.compile(source, extra=("-w", "all"))
    path = test.working / "diagnostic-binary.fbsem"
    test.invoke([source], mode="off", extra=("-w", "all", "-semantic-model", str(path)))
    binary = Model.read(path)
    test.assertEqual(text.diagnostic_contexts, binary.diagnostic_contexts)
    original = text.rows
    for mutation in ("missing-context", "native-counter", "ordinary-promoted", "kind", "missing-capability", "dangling", "duplicate"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in original]
            row = next(row for row in rows if row[:2] == ["K", "diagnostic"])
            if mutation == "missing-context":
                rows.remove(row)
            elif mutation == "native-counter":
                fields = row[4].split("\t")
                fields[3] = "2147483648"
                row[4] = "\t".join(fields)
            elif mutation == "ordinary-promoted":
                fields = row[4].split("\t")
                fields[6] = "1"
                row[4] = "\t".join(fields)
                next(item for item in rows if item[0] == "DI")[2] = "error"
            elif mutation == "kind":
                row[3] = "guaranteed-current"
            elif mutation == "missing-capability":
                next(item for item in rows if item[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            elif mutation == "dangling":
                row[2] = "999999"
            else:
                rows.insert(-1, row.copy())
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_parameter_kinds(test):
    cases = (
        ("constructor", 'type Resource\n declare constructor(byref value as long)\nend type\ndim item as Resource = Resource("wrong")\n', "RESOURCE"),
        ("procedure", 'type Wrapper\n declare sub Member(byref value as long)\nend type\ndim item as Wrapper\nitem.Member("wrong")\n', "WRAPPER"),
        ("procedure-pointer", 'dim cb as sub(byref value as long)\ncb("wrong")\n', ""),
    )
    for backend in test.backends:
        for kind, text, owner in cases:
            with test.subTest(backend=backend, kind=kind):
                source = test.source(text, "reported-" + kind + ".bas")
                _, diagnostic, _ = observe(test, source, backend=backend)
                frames = [frame for values in diagnostic.parameters.values() for frame in values]
                test.assertTrue(any(frame[6] == kind and frame[5] == owner and frame[2] == "1" for frame in frames))

def check_complete_counts(test):
    from driver_inputs import text_rows
    source = test.source("function CountedWarning() as long\nend function\n", "counted-warning.bas")
    model = test.compile(source, extra=("-w", "all"))
    test.assertEqual(model.diagnostic_totals[1], ["1", "0", "1", "0", "0", "accepted"])
    for mutation in ("missing-summary", "lost-diagnostic", "wrong-total", "wrong-module", "outcome"):
        with test.subTest(model_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            summary = next(row for row in rows if row[:2] == ["K", "module"])
            if mutation == "missing-summary":
                rows.remove(summary)
            elif mutation == "lost-diagnostic":
                rows = [row for row in rows if row[0] != "DI" and row[:2] != ["K", "diagnostic"]]
            elif mutation == "wrong-total":
                summary[4] = "0\t0\t0\t0\t0\taccepted"
            elif mutation == "wrong-module":
                summary[2] = "999999"
            else:
                summary[4] = "1\t0\t1\t0\t0\trejected"
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    invalid = test.source('declare sub CountedParameter(byref value as long)\nCountedParameter("wrong")\n', "counted-parameter.bas")
    _, diagnostic, _ = observe(test, invalid)
    from sidecar import unescape
    original = test.working / ("reported-" + str(test.sequence) + ".fbcdia")
    rows = [[unescape(field) for field in line.split("\t")] for line in original.read_text().splitlines()]
    def encoded(items):
        def escape(value):
            return "".join(chr(byte) if 32 <= byte < 127 and byte != 37 else f"%{byte:02X}"
                           for byte in value.encode("utf-8", errors="surrogateescape"))
        return "\n".join("\t".join(escape(field) for field in row) for row in items) + "\n"
    for mutation in ("missing-end", "wrong-count", "restarted-errors", "lost-diagnostic", "duplicate-end", "counter"):
        with test.subTest(artifact_mutation=mutation):
            changed = [row.copy() for row in rows]
            ending = next(row for row in changed if row[0] == "ME")
            if mutation == "missing-end":
                changed.remove(ending)
            elif mutation == "wrong-count":
                ending[3] = "2"
            elif mutation == "restarted-errors":
                ending[2:] = ["restarted", "0"]
            elif mutation == "lost-diagnostic":
                changed = [row for row in changed if row[0] not in ("DI", "DC", "DP")]
                changed[-1][4:6] = ["0", "0"]
            elif mutation == "duplicate-end":
                changed.insert(-1, ending.copy())
            else:
                context = next(row for row in changed if row[0] == "DC")
                fields = context[2].split("\t")
                fields[3] = "1"
                context[2] = "\t".join(fields)
            path = test.working / ("attempt-mutation-" + mutation + ".fbcdia")
            path.write_text(encoded(changed))
            with test.assertRaises(ValueError):
                Diagnostics.read(path)
    # Absence of both optional families is the older diagnostic transport.
    legacy = [row for row in rows if row[0] not in ("DC", "DP", "ME")
              and not (row[0] == "CAP" and row[1] == "0")]
    path = test.working / "legacy-diagnostics.fbcdia"
    path.write_text(encoded(legacy))
    old = Diagnostics.read(path)
    test.assertEqual(old.records, diagnostic.records)
    test.assertFalse(old.reported_contexts)
    test.assertFalse(old.attempt_lifetimes)

def check_collector_guards(test):
    from sidecar import unescape
    executable = test.working / ("context-guards.exe" if test.native_windows else "context-guards")
    sources = [test.root / "tests/semantic-sidecar/diagnostic-context-guards.bas"]
    sources += [test.root / "src/compiler" / unit for unit in (
        "tooling/semantic-diagnostics.bas", "tooling/semantic-output.bas",
        "support/containers/list.bas", "support/containers/hash.bas", "support/allocate.bas")]
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
               "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
               "-d", "BUILD_FB_VER_MAJOR=1", "-d", "BUILD_FB_VER_MINOR=20", "-d", "BUILD_FB_VER_PATCH=5",
               "-m", "diagnostic-context-guards", "-x", str(executable), *map(str, sources)]
    result = subprocess.run(command, cwd=test.working, text=True, capture_output=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, text=True, capture_output=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("DIAGNOSTIC_CONTEXT_GUARDS_PASS", result.stdout)
    diagnostic = Diagnostics.read(test.working / "context-guard.fbcdia")
    test.assertEqual(len(diagnostic.records), 7)
    test.assertEqual({frame[6] for frames in diagnostic.parameters.values() for frame in frames},
                     {"none", "unknown-symbol", "constructor", "destructor", "operator", "procedure-pointer", "procedure"})
    for frames in diagnostic.parameters.values():
        test.assertEqual(frames[0][3].encode("utf-8", errors="surrogateescape"), b"raw%\t\n\r\x01\xff")
    groups = [line.split("\t") for line in (test.working / "captured-context.tsv").read_text().splitlines()]
    parameters = [unescape(row[4]).split("\t") for row in groups if row[3].startswith("reported-parameter:")]
    test.assertEqual(len(parameters), 7)
    test.assertTrue(all(unescape(frame[3]).encode("utf-8", errors="surrogateescape") == b"raw%\t\n\r\x01\xff" for frame in parameters))
    invocation = Diagnostics.read(test.working / "invocation-guard.fbcdia")
    test.assertFalse(invocation.modules)
    test.assertEqual(invocation.records[0][2], "0")
    test.assertEqual(invocation.contexts[1][11], "0")

def check_warning_levels(test):
    source = test.source("function CatalogWarning() as long\nend function\n", "catalog-warning.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            _, diagnostic, path = observe(test, source, success=True, mode="expressions", backend=backend, extra=("-w", "all"))
            model = Model.read(path, expressions_only=True)
            test.assertTrue(diagnostic.warning_levels)
            test.assertEqual(len(diagnostic.records), 1)
            context = diagnostic.contexts[1]
            test.assertEqual((context[4], context[12]), ("0", "1"))
            test.assertEqual(list(model.diagnostic_contexts.values()), [context])
            _, hidden, _ = observe(test, source, success=True, backend=backend, extra=("-w", "2"))
            test.assertFalse(hidden.records)
    from driver_inputs import text_rows
    model = test.compile(source, extra=("-w", "all"))
    for mutation in ("missing", "zero", "negative", "too-large", "suppressed", "error-level"):
        with test.subTest(warning_level_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[:2] == ["K", "diagnostic"] and row[3] == "reported-context")
            fields = row[4].split("\t")
            if mutation == "missing":
                fields.pop()
            elif mutation == "suppressed":
                fields[4] = "2"
            elif mutation == "error-level":
                next(row for row in rows if row[0] == "DI")[2] = "error"
            else:
                fields[12] = {"zero": "0", "negative": "-2", "too-large": "4"}[mutation]
            row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of reported_diagnostics.py
