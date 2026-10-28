"""Project: FreeBASIC semantic sidecar tests
File: native_blocks.py
Purpose: Prove native block snapshots, exit ownership and cleanup selections.
Responsibilities: Target/order checks, deliberate omissions, runtime and corruptions.
This file intentionally does NOT reconstruct source exits from generated branches.
"""

import subprocess

from driver_inputs import text_rows
from semantic_blocks import FEATURE, FAMILIES, PREFIXES, PROVENANCE_FEATURE
from sidecar import Model


def check_structure(test):
    source = test.fixture("native-blocks.bas")
    choices = [(backend, ()) for backend in test.backends]
    choices += [("gas", ("-target", "linux-x86")), ("gas64", ())]
    for backend, extra in choices:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend, extra=extra)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            facts = model.native_blocks[1]
            proc = test.one(model, "NativeBlockExit", "procedure")
            unused = test.one(model, "UnusedBlockBody", "procedure")
            procedures = {int(row[0]): row for row in facts[FAMILIES[0]].values()}
            test.assertIn(unused, procedures)
            test.assertEqual(procedures[proc][5], "0")
            test.assertTrue(any(row[5] == "1" for row in procedures.values()))
            scopes = [row for row in facts[FAMILIES[1]].values() if row[2] == str(proc)]
            test.assertGreaterEqual(len(scopes), 5)
            test.assertTrue(any(row[1] in {value[0] for value in scopes} for row in scopes))
            exits = {identity: row for identity, row in facts[FAMILIES[2]].items() if row[0] == str(proc)}
            test.assertGreaterEqual(len(exits), 6)
            cleanup = {}
            for row in facts[FAMILIES[3]].values():
                cleanup.setdefault(int(row[0]), []).append(int(row[2]))
            expected = [test.one(model, name, "variable") for name in ("inner_block", "outer_block", "root_block")]
            test.assertTrue(any(cleanup.get(identity) == expected for identity in exits))
            plain = test.one(model, "plain_block", "variable")
            test.assertNotIn(plain, {int(row[2]) for row in facts[FAMILIES[3]].values()})
            for node, values in model.properties.items():
                if node[0] == "node" and "native-scope-exit" in values:
                    identity = int(values["native-scope-exit"])
                    native = facts[FAMILIES[2]][identity]
                    targets = {row[4] for row in model.relations("branch-target") if row[1:3] == ["node", str(node[1])]}
                    test.assertEqual(targets, {native[2]})
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_modes(test):
    source = test.fixture("native-blocks.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
                test.assertEqual(model.capabilities[1][PROVENANCE_FEATURE], "unavailable")
                test.assertFalse([row for row in model.records["K"] if row[3].startswith(PREFIXES)])
        for abi in ("native", "stable-v1"):
            with test.subTest(backend=backend, abi=abi):
                extra = ("-semantic-type-abi", abi)
                model = test.compile(source, backend=backend, extra=extra)
                path = test.working / ("blocks-" + backend + "-" + abi + ".bin")
                test.invoke([source], backend=backend, mode="off", extra=(*extra, "-semantic-model", str(path)))
                test.assertEqual(Model.read(path).rows, model.rows)
    first = test.source('sub BlockModuleOne()\nscope\ndim first_block_text as string = "first"\nend scope\nend sub\n', "block-one.bas")
    second = test.source('sub BlockModuleTwo()\nscope\ndim second_block_text as string = "second"\nend scope\nend sub\n', "block-two.bas")
    _, path = test.invoke([first, second])
    combined = Model.read(path)
    test.assertEqual(set(combined.native_blocks), {1, 2})
    for facts in combined.native_blocks.values():
        for family in FAMILIES[:2]:
            test.assertEqual(next(iter(facts[family])), 1)


def check_rejection(test):
    model = test.compile(test.fixture("native-blocks.bas"))
    plain = test.one(model, "plain_block", "variable")
    proc = test.one(model, "NativeBlockExit", "procedure")
    cases = (("native-procedure-block:", 5, "2"), ("native-procedure-block:", 1, str(proc)),
             ("native-procedure-block:", 3, "4294967295"), ("native-scope-block:", 1, str(plain)),
             ("native-scope-block:", 2, str(plain)), ("native-scope-exit:", 2, str(plain)),
             ("native-scope-exit:", 6, "4294967295"), ("native-exit-cleanup:", 1, "0"),
             ("native-exit-cleanup:", 2, str(plain)), ("native-exit-cleanup:", 3, str(plain)),
             ("native-scope-exit-end:", 0, "1000000"), ("native-block-counts:", 0, "0"))
    for prefix, slot, value in cases:
        with test.subTest(prefix=prefix, slot=slot):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            fields = row[4].split("\t")
            fields[slot] = value
            row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    for prefix in ("native-procedure-block:", "native-scope-block:", "native-scope-exit-end:", "native-block-counts:"):
        with test.subTest(remove=prefix):
            rows = [row.copy() for row in model.rows]
            removed = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            rows.remove(removed)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    # Recounting the module cannot hide a missing cleanup selected for an
    # exit, because its independent native break-list closure still owns it.
    rows = [row.copy() for row in model.rows]
    cleanup = [row for row in rows if row[0] == "K" and row[3].startswith("native-exit-cleanup:")]
    rows.remove(cleanup[-1])
    counts = next(row for row in rows if row[0] == "K" and row[3].startswith("native-block-counts:"))
    values = counts[4].split("\t")
    values[3] = str(int(values[3]) - 1)
    counts[4] = "\t".join(values)
    with test.assertRaises(ValueError):
        Model(text_rows(rows))
    for capability in ("unavailable", "partial"):
        rows = [row.copy() for row in model.rows]
        next(row for row in rows if row[0] == "CAP" and row[2] == FEATURE)[3] = capability
        with test.assertRaises(ValueError):
            Model(text_rows(rows))
    linked = next(row for row in model.records["K"] if row[1] == "node" and row[3] == "native-scope-exit")
    rows = [row.copy() for row in model.rows]
    next(row for row in rows if row == linked)[4] = "1000000"
    with test.assertRaises(ValueError):
        Model(text_rows(rows))
    # A recounted omission of a wrapper still contradicts the independent
    # body phase or scope node. Footer totals alone cannot prove this link.
    for family in FAMILIES[:2]:
        rows = [row.copy() for row in model.rows]
        selected = [row for row in rows if row[0] == "K" and row[3].startswith(family + ":")]
        rows.remove(selected[-1])
        closure = next(row for row in rows if row[0] == "K" and row[3].startswith("native-block-counts:"))
        fields = closure[4].split("\t")
        fields[FAMILIES.index(family)] = str(int(fields[FAMILIES.index(family)]) - 1)
        closure[4] = "\t".join(fields)
        with test.assertRaises(ValueError):
            Model(text_rows(rows))


def check_execution(test):
    baseline = None
    for mode in ("off", "full", "bindings", "expressions"):
        executable = test.working / ("blocks-" + mode)
        test.invoke([test.fixture("native-blocks.bas")], mode=mode, emit=False, extra=("-x", str(executable)))
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        test.assertEqual([int(value) for value in result.stdout.split()], [30, 20, 10, 7])
        if baseline is None:
            baseline = result.stdout
        else:
            test.assertEqual(result.stdout, baseline)


def check_guards(test):
    executable = test.working / "block-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("native-block-guards.bas")), str(test.root / "src/compiler/tooling/semantic-blocks.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "BLOCK_GUARDS_PASS")


def check_origins(test):
    source = test.fixture("native-blocks.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            test.assertEqual(model.capabilities[1][PROVENANCE_FEATURE], "available")
            proc = test.one(model, "NativeBlockExit", "procedure")
            origins = {int(row[1]): row for row in model.native_block_origins[1].values()}
            test.assertTrue(all(int(value) > 0 for value in origins[proc][2:]))
            for scope in model.native_blocks[1][FAMILIES[1]].values():
                if scope[2] == str(proc):
                    origin = origins[int(scope[0])]
                    test.assertEqual(origin[0], "scope")
                    test.assertGreater(int(origin[2]), 0)
            cleanups = model.native_blocks[1][FAMILIES[3]]
            selected = {identity for identity, row in cleanups.items()
                        if model.native_blocks[1][FAMILIES[2]][int(row[0])][0] == str(proc)}
            observed = list(model.native_cleanup_nodes[1].values())
            test.assertTrue(selected)
            test.assertTrue(selected <= set(observed))
            test.assertTrue(all(observed.count(identity) == 1 for identity in selected))
    header = test.source('scope\ndim included_block as string = "value"\nend scope\n', "block-origin.bi")
    owner = test.source('sub IncludedBlockOrigins()\n#include "block-origin.bi"\n#include "block-origin.bi"\nend sub\n', "block-origins.bas")
    model = test.compile(owner)
    proc = test.one(model, "IncludedBlockOrigins", "procedure")
    scopes = {int(row[0]) for row in model.native_blocks[1][FAMILIES[1]].values() if row[2] == str(proc)}
    origins = [row for row in model.native_block_origins[1].values() if row[0] == "scope" and int(row[1]) in scopes]
    test.assertEqual(len(origins), 2)
    test.assertEqual(len({row[3] for row in origins}), 2)
    for row in origins:
        source = model.source_contexts[int(row[3])]
        test.assertEqual(model.files[int(source[3])][2], test.compiler_path(header))


def check_origin_rejection(test):
    model = test.compile(test.fixture("native-blocks.bas"))
    cases = (("native-block-origin:", 0, "guessed"), ("native-block-origin:", 1, "4294967295"),
             ("native-block-origin:", 2, "4294967295"), ("native-block-origin:", 3, "4294967295"),
             ("native-block-origin:", 4, "4294967295"), ("native-block-provenance-counts:", 1, "0"),
             ("native-cleanup-insertion:", 0, "2"))
    for prefix, slot, value in cases:
        with test.subTest(prefix=prefix, slot=slot):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            fields = row[4].split("\t")
            fields[slot] = value
            row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    for domain, key, slot in (("symbol", "native-block-origin:", 0), ("node", "native-exit-cleanup", 1)):
        with test.subTest(recounted_missing=key):
            rows = [row.copy() for row in model.rows]
            victim = next(row for row in rows if row[0] == "K" and row[1] == domain and row[3].startswith(key))
            rows.remove(victim)
            closure = next(row for row in rows if row[0] == "K" and row[3].startswith("native-block-provenance-counts:"))
            fields = closure[4].split("\t")
            fields[slot] = str(int(fields[slot]) - 1)
            closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    rows = [row.copy() for row in model.rows]
    next(row for row in rows if row[0] == "K" and row[1] == "node" and row[3] == "native-exit-cleanup")[4] = "1000000"
    with test.assertRaises(ValueError):
        Model(text_rows(rows))

# end of native_blocks.py
