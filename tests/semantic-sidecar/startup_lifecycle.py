"""Project: FreeBASIC semantic sidecar tests
File: startup_lifecycle.py
Purpose: Prove actual startup lists, wrapper ownership and guarded static paths.
Responsibilities: Priorities, order, backend choices, omissions and corruption.
This file intentionally does NOT infer runtime loader tie order from symbol IDs.
"""

import subprocess

from driver_inputs import text_rows
from semantic_startup import FEATURE
from sidecar import Model


def fields_for(model, family, symbol, slot=0):
    return [fields for fields in model.startup_lifecycles[1][family].values() if fields[slot] == str(symbol)]


def check_lists_and_instances(test):
    source = test.fixture("startup-lifecycle.bas")
    choices = [(backend, ()) for backend in test.backends]
    choices += [("gas", ("-target", "linux-x86")), ("gas64", ())]
    for backend, extra in choices:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend, extra=extra)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            facts = model.startup_lifecycles[1]
            expected = [test.one(model, name, "procedure") for name in ("EarlyStartup", "SameStartupA", "SameStartupB")]
            source_order = [int(row[0]) for row in facts["startup-registration"].values() if int(row[0]) in expected]
            test.assertEqual(source_order, expected)
            for kind in ("constructor", "destructor"):
                entries = model.startup_registration_lists[1, kind]
                test.assertEqual([entry[1] for entry in entries],
                                 [int(fields[0]) for fields in facts["startup-registration"].values() if fields[1] == kind])
                test.assertEqual([entry[2] for entry in entries], [entry[0] for entry in entries[1:]] + ([0] if entries else []))
            test.assertEqual([fields_for(model, "startup-priority-input", identity)[-1][1] for identity in expected], ["101", "201", "201"])
            global_symbols = [test.one(model, name, "variable") for name in ("first_startup", "second_startup")]
            instances = {int(row[0]): identity for identity, row in facts["global-instance"].items()}
            identities = [instances[symbol] for symbol in global_symbols]
            initialized = [int(row[0]) for row in facts["global-instance-action"].values() if row[2] == "initialize"]
            cleanup = [int(row[0]) for row in facts["global-instance-action"].values() if row[2] == "cleanup"]
            test.assertEqual(initialized, identities)
            test.assertEqual(cleanup, identities[::-1])
            plain = test.one(model, "plain_startup", "variable")
            test.assertNotIn(plain, instances)
            local = test.one(model, "local_startup", "variable")
            scalar = test.one(model, "scalar_startup", "variable")
            # A dynamic array and its descriptor can share the source name.
            # The native A record identifies the array rather than its storage.
            array_symbols = {int(row[1]) for row in model.records["A"]}
            arrays = [identity for identity in model.named("array_startup", "variable") if identity in array_symbols]
            test.assertEqual(len(arrays), 1)
            array = arrays[0]
            local_path = fields_for(model, "static-instance", local)
            test.assertEqual(len(local_path), 1)
            test.assertEqual(local_path[0][3:6], ("1", "1", "1"))
            test.assertNotEqual(local_path[0][2], "0")
            scalar_path = fields_for(model, "static-instance", scalar)
            test.assertEqual(scalar_path[0][2:6], ("0", "1", "0", "0"))
            array_paths = fields_for(model, "static-instance", array)
            test.assertTrue(any(row[6] == "array-allocation" for row in array_paths))
            test.assertTrue(all(row[2] == "0" for row in array_paths if row[6] == "array-allocation"))
            parent = test.one(model, "UseStartupStatics", "procedure")
            test.assertTrue(all(row[1] == str(parent) for row in local_path + scalar_path + array_paths))
            test.assertIn((local, int(local_path[0][2])), model.static_cleanup_members[1, parent])
            test.assertNotIn((scalar, 0), model.static_cleanup_members[1, parent])
            guard_instances = {int(row[0]) for row in facts["static-instance-guard"].values()}
            for identity, row in facts["static-instance"].items():
                if row[3] == "1" or row[5] == "1":
                    test.assertIn(identity, guard_instances)
            assigned = test.one(model, "AssignedStartupResult", "procedure")
            returned = test.one(model, "ReturnedStartupResult", "procedure")
            test.assertEqual(model.properties["symbol", assigned]["result-assignment-used"], "1")
            test.assertEqual(model.properties["symbol", assigned]["return-used"], "0")
            test.assertEqual(model.properties["symbol", returned]["return-used"], "1")
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_unemitted_membership(test):
    source = test.source('''type CleanupItem
    value as long
    declare destructor()
end type
destructor CleanupItem()
end destructor
private sub NeverCalled()
    static item as CleanupItem
end sub
print 1
''', "startup-unemitted-membership.bas")
    for backend in test.backends:
        model = test.compile(source, backend=backend)
        parent = test.one(model, "NeverCalled", "procedure")
        entries = model.static_cleanup_members[1, parent]
        test.assertEqual(len(entries), 1)
        variable, wrapper = entries[0]
        test.assertEqual(model.symbols[variable][3], "1")
        test.assertEqual(model.symbols[wrapper][3], "3")
        emitted = {int(row[2]) for row in model.records["PH"] if row[3:5] == ["pre-load", "1"]}
        test.assertNotIn(parent, emitted)
        bodies = model.startup_lifecycles[1]["static-cleanup-body"]
        test.assertFalse(any(row[2] == str(wrapper) for row in bodies.values()))
        # An allocated wrapper is retained as membership even though neither
        # the uncalled parent nor that wrapper has an emitted cleanup body.


def check_backend_priorities(test):
    source = test.fixture("startup-lifecycle.bas")
    choices = [(backend, target, ()) for backend in test.backends
               for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64", "netbsd-x86_64")]
    choices += [("gas", "linux-x86", ()), ("gas64", "linux-x86_64", ()), ("gas64", "netbsd-x86_64", ())]
    for backend, target, more in choices:
        with test.subTest(backend=backend, target=target):
            extra = ("-target", target, *more)
            test.invoke([source], mode="off", backend=backend, extra=extra)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            observed = model.startup_lifecycles[1]["startup-emission"]
            test.assertTrue(observed)
            text = emission.decode("ascii")
            for row in observed.values():
                test.assertIn(row[7], text)
            early = test.one(model, "EarlyStartup", "procedure")
            entries = fields_for(model, "startup-emission", early)
            test.assertTrue(entries)
            test.assertTrue(all(row[4] == "101" for row in entries))
            if backend == "llvm":
                test.assertTrue(all(row[5] == "101" and row[6] == "llvm.global_ctors" for row in entries))
                defaults = [row for row in observed.values() if row[4] == "0"]
                test.assertTrue(defaults)
                test.assertTrue(all(row[5] == "65535" for row in defaults))
            elif backend in ("gcc", "clang") and target.startswith("netbsd"):
                late = test.one(model, "LateTeardown", "procedure")
                finalizers = fields_for(model, "startup-emission", late)
                test.assertTrue(all(row[3] == "array-entry" and row[6] == ".fini_array.65434" for row in finalizers))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_modes_and_transport(test):
    source = test.fixture("startup-lifecycle.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(model.startup_lifecycles)
    for abi in ("native", "stable-v1"):
        text = test.compile(source, extra=("-semantic-type-abi", abi))
        artifact = test.working / ("startup-" + abi + ".bin")
        test.invoke([source], mode="off", extra=("-semantic-type-abi", abi, "-semantic-model", str(artifact),
                                                "-o", str(source.with_suffix(".o"))))
        test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source("sub FirstStartup() constructor 101\nend sub\n", "first-startup.bas")
    second = test.source("sub SecondStartup() constructor 201\nend sub\n", "second-startup.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    test.assertEqual(set(model.startup_lifecycles), {1, 2})
    test.assertEqual(next(iter(model.startup_lifecycles[2]["startup-registration"])), 1)


def check_rejection(test):
    model = test.compile(test.fixture("startup-lifecycle.bas"), backend="llvm")
    mutations = ("registration-proc", "registration-kind", "registration-order", "priority", "global-variable", "global-cleanup",
                 "static-parent", "static-wrapper", "static-registration", "static-purpose", "guard-path", "guard-condition",
                 "wrapper-kind", "action-instance", "action-kind", "action-order", "body-parent", "emission-stage", "emission-kind",
                 "emission-priority", "emission-placement", "count", "missing-guard", "missing-action", "missing-wrapper", "unavailable")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def packet(prefix, predicate=lambda values: True):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(prefix)
                            and predicate(row[4].split("\t")))
            if mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            elif mutation.startswith("missing-"):
                prefix = {"missing-guard": "static-instance-guard:", "missing-action": "global-instance-action:",
                          "missing-wrapper": "startup-instance-wrapper:"}[mutation]
                rows.remove(packet(prefix))
            else:
                family, slot = {
                    "registration-proc": ("startup-registration", 0), "registration-kind": ("startup-registration", 1),
                    "registration-order": ("startup-registration", 2), "priority": ("startup-priority-input", 1),
                    "global-variable": ("global-instance", 0), "global-cleanup": ("global-instance", 2),
                    "static-parent": ("static-instance", 1), "static-wrapper": ("static-instance", 2),
                    "static-registration": ("static-instance", 5), "static-purpose": ("static-instance", 6),
                    "guard-path": ("static-instance-guard", 0), "guard-condition": ("static-instance-guard", 3),
                    "wrapper-kind": ("startup-instance-wrapper", 1), "action-instance": ("global-instance-action", 0),
                    "action-kind": ("global-instance-action", 2), "action-order": ("global-instance-action", 3),
                    "body-parent": ("static-cleanup-body", 1), "emission-stage": ("startup-emission", 2),
                    "emission-kind": ("startup-emission", 1), "emission-priority": ("startup-emission", 5),
                    "emission-placement": ("startup-emission", 6), "count": ("startup-lifecycle-counts", 0)}[mutation]
                row = packet(family + ":")
                fields = row[4].split("\t")
                fields[slot] = {"registration-kind": "pretend", "global-cleanup": "0", "static-registration": "0",
                                "static-purpose": "pretend", "guard-condition": "nonzero", "wrapper-kind": "destructor",
                                "action-kind": "pretend", "emission-stage": "c", "emission-kind": "destructor",
                                "emission-placement": "llvm.global_dtors"}.get(mutation, "999999")
                row[4] = "\t".join(fields)
            Model(text_rows(rows))


def check_execution(test):
    source = test.fixture("startup-lifecycle.bas")
    baseline = None
    for mode in ("off", "full", "bindings", "expressions"):
        with test.subTest(mode=mode):
            executable = test.working / ("startup-" + mode)
            test.invoke([source], mode=mode, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            lines = result.stdout.splitlines()
            test.assertEqual(lines.count("I10"), 1)
            test.assertEqual(lines.count("D10"), 1)
            test.assertEqual([row for row in lines if row in ("I1", "I2")], ["I1", "I2"])
            test.assertEqual([row for row in lines if row in ("D1", "D2")], ["D2", "D1"])
            test.assertNotIn("I20", lines)
            if baseline is None:
                baseline = result.stdout
            else:
                test.assertEqual(result.stdout, baseline)


def check_guards(test):
    executable = test.working / "startup-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("startup-guards.bas")), str(test.root / "src/compiler/tooling/semantic-startup.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "STARTUP_GUARDS_PASS")


def check_legacy_order_and_shared(test):
    source = test.source('#lang "fblite"\noption gosub\nsub LegacyStartupOrder()\n'
                         'dim first_declared as integer\n'
                         'goto SkipDeclaration\n'
                         'dim crossed_declared as integer\n'
                         'SkipDeclaration:\n'
                         'dim last_declared as integer\n'
                         'gosub VisitStartupLabel\n'
                         'exit sub\n'
                         'VisitStartupLabel:\n'
                         'return\nend sub\nLegacyStartupOrder()\n', "startup-legacy.bas")
    choices = [(backend, ()) for backend in test.backends]
    choices += [("gas", ("-target", "linux-x86")), ("gas64", ()), ("gas64", ("-z", "gosub-setjmp"))]
    for backend, extra in choices:
        with test.subTest(backend=backend, route="legacy", options=extra):
            test.invoke([source], mode="off", backend=backend, extra=extra)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            variables = [test.one(model, name, "variable") for name in ("first_declared", "crossed_declared", "last_declared")]
            # Unscoped FBLite declarations can have equal native statement
            # numbers. The explicit accepted-input sequence preserves order.
            native_order = [int(model.properties["symbol", identity]["declaration-statement"]) for identity in variables]
            test.assertTrue(all(value >= 0 for value in native_order))
            declaration_order = {int(row[0]): ordinal for ordinal, row in model.declarator_orders[1].items()}
            order = [declaration_order[identity] for identity in variables]
            test.assertEqual(order, sorted(set(order)))
            label = test.one(model, "VisitStartupLabel", "label")
            test.assertEqual(model.properties["symbol", label]["declared-label"], "1")
            # Native lbl.gosub marks the assembler landing pad. The setjmp
            # route still has a source GOSUB, without that extra label frame.
            landing_pad = backend in ("gas", "gas64") and "gosub-setjmp" not in extra
            test.assertEqual(model.properties["symbol", label]["gosub-label"], "1" if landing_pad else "0")
            test.assertTrue(any(row[2] == "gosub" for row in model.records["SOP"]))
            test.assertTrue(any(values.get("gosub-used") == "1" for (domain, _), values in model.properties.items() if domain == "symbol"))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
    shared = test.fixture("startup-lifecycle.bas")
    for backend in (*test.backends, "gas64"):
        with test.subTest(backend=backend, route="shared"):
            extra = ("-dll",)
            test.invoke([shared], mode="off", backend=backend, extra=extra)
            emission = test.emission_path(shared.stem, backend).read_bytes()
            model = test.compile(shared, backend=backend, extra=extra)
            entries = model.startup_lifecycles[1]["startup-emission"]
            test.assertTrue(entries)
            if backend == "gas64":
                test.assertTrue(all(row[6] in (".init_array", ".fini_array") for row in entries.values()))
            test.assertEqual(test.emission_path(shared.stem, backend).read_bytes(), emission)


def check_declarator_integrity(test):
    source = test.source("dim first_order as long, second_order as double\nconst third_order as long = 3\n", "declarator-order.bas")
    model = test.compile(source)
    expected = [test.one(model, "first_order", "variable"), test.one(model, "second_order", "variable"),
                test.one(model, "third_order", "constant")]
    selected = [int(row[0]) for row in model.declarator_orders[1].values() if int(row[0]) in expected]
    test.assertEqual(selected, expected)
    for mutation in ("symbol", "group", "item", "statement", "source", "missing-recounted", "unavailable"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            if mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", "accepted-declarator-order"])[3] = "unavailable"
            else:
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("accepted-declarator-order:"))
                if mutation == "missing-recounted":
                    rows.remove(row)
                    for ordinal, value in enumerate((value for value in rows if value[0] == "K" and value[3].startswith("accepted-declarator-order:")), 1):
                        value[3] = "accepted-declarator-order:" + str(ordinal)
                    count = next(value for value in rows if value[0] == "K" and value[3] == "accepted-declarator-count:1")
                    count[4] = str(int(count[4]) - 1)
                else:
                    fields = row[4].split("\t")
                    fields[{"symbol": 0, "group": 2, "item": 3, "statement": 4, "source": 5}[mutation]] = "999999"
                    row[4] = "\t".join(fields)
            Model(text_rows(rows))

# end of startup_lifecycle.py
