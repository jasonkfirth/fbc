"""Project: FreeBASIC semantic sidecar tests
File: layout_observations.py
Purpose: Prove packing provenance and each native aggregate layout operation.
Responsibilities: Target partitions, storage classes, source contexts and corruption.
This file intentionally does NOT equate packing with one universal object alignment.
"""

import subprocess

from driver_inputs import text_rows
from named_flags import flag_names
from semantic_layout import FEATURE, PACKING_FEATURE, FAMILIES
from sidecar import Model


def packets(model, family, identity):
    return [list(values) for values in model.aggregate_layouts[1][family].values() if values[0] == str(identity)]


def check_requests(test):
    source = test.fixture("layout-requests.bas")
    requests = {"NaturalLayout": (None, 0), "PackedLayout": (1, 1), "CoercedLayout": (3, 2),
                "ZeroLayout": (0, 0), "NegativeLayout": (-1, 0), "ExcessiveLayout": (16, 0)}
    for backend in test.backends:
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            with test.subTest(backend=backend, target=target):
                extra = ("-target", target)
                test.invoke([source], mode="off", backend=backend, extra=extra)
                emission = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                for name, (requested, selected) in requests.items():
                    identity = test.one(model, name, "type")
                    request, = packets(model, "packing", identity)
                    test.assertEqual(request[4], "0" if requested is None else "1")
                    test.assertEqual(request[6], "" if requested is None else str(requested))
                    test.assertEqual(request[7:10], [str(selected)] * 3)
                    test.assertGreater(int(request[1]), 0)
                    test.assertGreater(int(request[2]), 0)
                    if requested is not None:
                        test.assertGreater(int(request[5]), 0)
                odd, = packets(model, "packing", test.one(model, "OddLayout", "type"))
                test.assertEqual(odd[6:10], ["5"] * 4 if target.endswith("x86_64") else ["5", "0", "0", "0"])
                nat = "4" if target == "linux-x86" else "8"
                packed, = packets(model, "end", test.one(model, "PackedLayout", "type"))
                test.assertEqual(packed[9:13], ["9", "9", nat, "1"])
                test.assertFalse(model.named("NeverLayout", "type"))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
                for mode in ("bindings", "expressions"):
                    reduced = test.compile(source, backend=backend, mode=mode, extra=extra)
                    test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                    test.assertEqual(reduced.capabilities[1][PACKING_FEATURE], "unavailable")
                    test.assertFalse(reduced.aggregate_layouts)
                    test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_storage(test):
    source = test.fixture("layout-requests.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            pointer_bytes = int(model.contexts[0][6])
            facts = model.aggregate_layouts[1]
            states = model.aggregate_layout_subjects[1]
            test.assertEqual({value[1] for value in states.values()}, {"created", "preexisting"})
            managed = test.one(model, "ManagedLayout", "type")
            storage = packets(model, "field", managed)
            logical, = [value for value in storage if value[2] == "dynamic-alias"]
            descriptor, = [value for value in storage if value[1] == logical[19]]
            text, = [value for value in storage if model.symbols[int(value[1])][2].casefold() == "text"]
            for selected in (text, descriptor):
                test.assertEqual(selected[4], "1")
                test.assertEqual(selected[7:10], [str(pointer_bytes), str(pointer_bytes), "managed-descriptor"])
            test.assertIn("array-descriptor", flag_names(model, int(descriptor[1]), "attributes"))
            test.assertEqual(logical[9], "none")
            test.assertFalse(any(row[1] == descriptor[1] for row in model.records["B"]))
            array = packets(model, "field", test.one(model, "ArrayLayout", "type"))
            item, = [value for value in array if model.symbols[int(value[1])][2].casefold() == "items"]
            pointers, = [value for value in array if model.symbols[int(value[1])][2].casefold() == "pointers"]
            test.assertEqual(item[14:17], ["4", "2", "12"])
            test.assertEqual(pointers[14], str(pointer_bytes))
            test.assertEqual(pointers[16], str(2 * pointer_bytes))
            bitfields = packets(model, "field", test.one(model, "BitLayout", "type"))
            test.assertTrue(any(value[2] == "shared-bitfield" and int(value[17]) > 0 for value in bitfields))
            test.assertTrue(any(int(value[18]) and int(value[10]) > int(value[11]) for value in bitfields))
            child = test.one(model, "ChildLayout", "type")
            snapshots = packets(model, "snapshot", child)
            base = int(snapshots[-1][6])
            test.assertGreater(base, 0)
            test.assertEqual(model.symbols[base][5], str(test.one(model, "ParentLayout", "type")))
            test.assertTrue(facts["insert"])
            test.assertTrue(facts["promote"])
            test.assertTrue(any(value[16] == "1" for value in facts["insert"].values()))
            test.assertTrue(any(value[16] == "0" for value in facts["insert"].values()))
            self_type = test.one(model, "SelfLayout", "type")
            self_snapshots = packets(model, "snapshot", self_type)
            test.assertTrue(any(value[5] == "0" for value in self_snapshots))
            test.assertEqual(self_snapshots[-1][5], "1")
            scoped = test.source("#pragma push(msbitfields)\n" + source.read_text() + "\n#pragma pop(msbitfields)\n", "ms-layout.bas")
            ms = test.compile(scoped, backend=backend)
            bitfields = packets(ms, "field", test.one(ms, "BitLayout", "type"))
            test.assertTrue(all(value[10] == value[11] for value in bitfields))
    runnable = test.source("type RecordLayout field = 1\na as ubyte\nb as long\nend type\n"
                           "dim values(0 to 1) as RecordLayout\nvalues(1).b = 17\n"
                           "if sizeof(RecordLayout) <> 5 or values(1).b <> 17 then end 1\nend 0\n", "layout-runtime.bas")
    for mode in ("off", "full"):
        executable = test.working / ("layout-runtime-" + mode)
        test.invoke([runnable], mode=mode, emit=False, extra=("-x", str(executable)))
        result = subprocess.run([str(executable)], capture_output=True, timeout=30)
        test.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))


def check_contexts(test):
    source = test.source("__FB_UNIQUEID_PUSH__(PACK_STATE)\n"
                         "dim __FB_UNIQUEID__(PACK_STATE) as long = 7\n"
                         "__FB_UNIQUEID_PUSH__(PACK_STATE)\n"
                         "dim __FB_UNIQUEID__(PACK_STATE) as long = 11\n"
                         "type CountedLayout field = __FB_UNIQUEID_POP__(PACK_STATE) 1\nvalue as long\nend type\n"
                         "print __FB_UNIQUEID__(PACK_STATE)\n__FB_UNIQUEID_POP__(PACK_STATE)\n"
                         "namespace First\n#include \"packing.bi\"\nend namespace\n"
                         "namespace Second\n#include \"packing.bi\"\nend namespace\n")
    test.source("type RepeatedLayout field = 2\nvalue as long\nend type\n", "packing.bi")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            request, = packets(model, "packing", test.one(model, "CountedLayout", "type"))
            test.assertEqual(request[6:10], ["1"] * 4)
            callbacks = [model.macro_definitions[int(model.macro_invocations[identity][3])][5]
                         for identity in model.macro_callbacks]
            test.assertEqual(callbacks.count("__FB_UNIQUEID_PUSH__"), 2)
            test.assertEqual(callbacks.count("__FB_UNIQUEID_POP__"), 2)
            repeated = [value for value in model.aggregate_layouts[1]["packing"].values()
                        if model.symbols[int(value[0])][2].casefold() == "repeatedlayout"]
            test.assertEqual(len(repeated), 2)
            test.assertNotEqual(repeated[0][2], repeated[1][2])
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
    qb = test.source("type DefaultRecord\na as string * 1\nb as long\nend type\n"
                     "type ZeroRecord field = 0\na as long\nend type\n", "packing-qb.bas")
    model = test.compile(qb, extra=("-lang", "qb"))
    default, = packets(model, "packing", test.one(model, "DefaultRecord", "type"))
    explicit, = packets(model, "packing", test.one(model, "ZeroRecord", "type"))
    test.assertEqual(default[4:10], ["0", "0", "", "0", "0", "1"])
    test.assertEqual(explicit[4], "1")
    test.assertEqual(explicit[6:10], ["0", "0", "0", "1"])


def check_transport(test):
    source = test.fixture("layout-requests.bas")
    text = test.compile(source)
    artifact = test.working / "layout.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(artifact), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source("type FirstLayout field = 1\nvalue as long\nend type\n", "first-layout.bas")
    second = test.source("type SecondLayout field = 2\nvalue as long\nend type\n", "second-layout.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    test.assertEqual(set(model.aggregate_layouts), {1, 2})
    test.assertEqual(list(model.aggregate_layouts[2]["packing"]), [1])
    test.assertEqual(list(model.aggregate_layouts[2]["begin"])[0], 1)
    restarted = test.source("type BeforeRestart field = 1\nvalue as long\nend type\n"
                            "#ifndef LAYOUT_RESTARTED\n#cmdline \"-d LAYOUT_RESTARTED -restart\"\n#endif\n"
                            "type AfterRestart field = 2\nvalue as long\nend type\n", "layout-restart.bas")
    reset = test.compile(restarted)
    test.assertEqual(len(reset.records["M"]), 1)
    test.assertEqual(list(reset.aggregate_layouts[1]["packing"]), [1, 2])
    broken = test.source("type BrokenLayout field = 1\nvalue as long\n", "layout-broken.bas")
    plain, _ = test.invoke([broken], mode="off", success=False)
    observed, artifact = test.invoke([broken], success=False)
    test.assertEqual((observed.stdout, observed.stderr), (plain.stdout, plain.stderr))
    with test.assertRaises(ValueError):
        Model.read(artifact)


def check_architectures(test):
    source = test.source("type TargetLayout\nmarker as ubyte\nnumber as double\nwide as integer<128>\nend type\n")
    cases = (("amiga-m68k", "big", "2", "2"), ("riscos-arm", "little", "4", "16"),
             ("wince-arm", "little", "8", "8"), ("linux-aarch64", "little", "8", "16"))
    for target, byte_order, double_alignment, wide_alignment in cases:
        with test.subTest(target=target):
            extra = ("-target", target)
            test.invoke([source], mode="off", extra=extra)
            emission = test.emission_path(source.stem, "gcc").read_bytes()
            model = test.compile(source, extra=extra)
            test.assertEqual(model.contexts[0][7], byte_order)
            fields = packets(model, "field", test.one(model, "TargetLayout", "type"))
            for name, alignment in (("number", double_alignment), ("wide", wide_alignment)):
                field, = [value for value in fields if model.symbols[int(value[1])][2].casefold() == name]
                test.assertEqual(field[7:10], [alignment, alignment, "natural"])
            test.assertEqual(test.emission_path(source.stem, "gcc").read_bytes(), emission)
            if target == "amiga-m68k":
                primitives = {row[2]: row for row in model.primitives[0].values()}
                test.assertEqual(primitives["pointer"][4:6], ["4", "2"])
                test.assertEqual(primitives["string"][4:6], ["12", "2"])
    qb = test.source("type JSLayout\nmarker as string * 1\nvalue as double\nend type\n", "js-layout.bas")
    # This native driver's target table does not expose the separate JS
    # compiler variant. Its rejection cannot become a completed layout model.
    result, artifact = test.invoke([qb], success=False, extra=("-lang", "qb", "-target", "js"))
    test.assertIn("Invalid command-line option", result.stdout + result.stderr)
    with test.assertRaises(ValueError):
        Model.read(artifact)


def check_rejection(test):
    model = test.compile(test.fixture("layout-requests.bas"))
    mutations = ("request", "selected", "parent", "expression", "creation", "field-owner", "field-type",
                 "field-kind", "effective", "computed-padding", "applied-padding", "natural", "extent",
                 "relocation", "relocation-count", "tail", "snapshot", "base-storage", "subject",
                 "missing-field", "duplicate-relocation", "missing-relocation", "missing-snapshot",
                 "incomplete-finalization", "ordinal", "count", "unavailable")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def packet(family, predicate=lambda fields: True):
                prefix = "aggregate-packing-input:" if family == "packing" else "aggregate-layout-" + family + ":"
                return next(row for row in rows if row[0] == "K" and row[3].startswith(prefix) and predicate(row[4].split("\t")))
            if mutation in ("duplicate-relocation", "missing-relocation"):
                groups = {}
                for value in rows:
                    if value[0] == "K" and value[3].startswith("aggregate-layout-promote:"):
                        groups.setdefault(value[4].split("\t")[0], []).append(value)
                insertion, moved = next((identity, values) for identity, values in groups.items() if len(values) > 1)
                if mutation == "duplicate-relocation":
                    moved[1][4] = moved[0][4]
                else:
                    rows.remove(moved[1])
                    following = [value for value in rows if value[0] == "K" and value[3].startswith("aggregate-layout-promote:")]
                    for ordinal, value in enumerate(following, 1):
                        value[3] = "aggregate-layout-promote:" + str(ordinal)
                    insert = next(value for value in rows if value[0] == "K" and value[3] == "aggregate-layout-insert:" + insertion)
                    owner = insert[4].split("\t")[0]
                    for value in rows:
                        if value[0] != "K":
                            continue
                        parts = value[4].split("\t")
                        slot = None
                        if value[3] == "aggregate-layout-counts:1":
                            slot = 4
                        elif value[3] == "aggregate-layout-insert-result:" + insertion:
                            slot = 3
                        elif value[3].startswith("aggregate-layout-subject:") and parts[0] == owner:
                            slot = 7
                        if slot is not None:
                            parts[slot] = str(int(parts[slot]) - 1)
                            value[4] = "\t".join(parts)
                Model(text_rows(rows))
                continue
            elif mutation == "missing-snapshot":
                row = packet("snapshot", lambda fields: fields[5] == "0")
                rows.remove(row)
                Model(text_rows(rows))
                continue
            elif mutation == "incomplete-finalization":
                row = packet("snapshot", lambda fields: fields[5] == "0")
                fields = row[4].split("\t")
                fields[5] = "1"
            elif mutation in ("request", "selected", "parent", "expression"):
                row = packet("packing", lambda fields: fields[4] == "1")
                fields = row[4].split("\t")
                slot = {"request": 6, "selected": 7, "parent": 10, "expression": 5}[mutation]
                fields[slot] = {"request": "16", "selected": "255", "parent": "999999", "expression": "999999"}[mutation]
            elif mutation in ("field-owner", "field-type", "field-kind", "effective", "computed-padding", "applied-padding", "natural", "extent"):
                row = packet("field", lambda fields: fields[2] == "allocated")
                fields = row[4].split("\t")
                slot = {"field-owner": 1, "field-type": 12, "field-kind": 2, "effective": 8,
                        "computed-padding": 10, "applied-padding": 11, "natural": 22, "extent": 21}[mutation]
                fields[slot] = "dynamic-alias" if mutation == "field-kind" else "999999"
            elif mutation == "missing-field":
                row = packet("field", lambda fields: fields[2] == "dynamic-alias")
                owner = row[4].split("\t")[0]
                rows.remove(row)
                following = [value for value in rows if value[0] == "K" and value[3].startswith("aggregate-layout-field:")]
                for ordinal, value in enumerate(following, 1):
                    value[3] = "aggregate-layout-field:" + str(ordinal)
                for value in rows:
                    if value[0] != "K":
                        continue
                    parts = value[4].split("\t")
                    if value[3] == "aggregate-layout-counts:1":
                        parts[2] = str(int(parts[2]) - 1)
                        value[4] = "\t".join(parts)
                    elif value[3].startswith("aggregate-layout-subject:") and parts[0] == owner:
                        parts[5] = str(int(parts[5]) - 1)
                        value[4] = "\t".join(parts)
                Model(text_rows(rows))
                continue
            else:
                family = {"creation": "begin", "relocation": "promote", "relocation-count": "insert-result",
                          "tail": "end", "snapshot": "snapshot", "base-storage": "snapshot",
                          "subject": "subject", "ordinal": "field", "count": "counts"}.get(mutation)
                if mutation == "unavailable":
                    next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
                    Model(text_rows(rows))
                    continue
                row = packet(family, lambda fields: mutation != "base-storage" or fields[6] != "0")
                fields = row[4].split("\t")
                if mutation == "ordinal":
                    row[3] += ":invalid"
                else:
                    slot = {"creation": 2, "relocation": 3, "relocation-count": 3, "tail": 10,
                            "snapshot": 1, "base-storage": 6, "subject": 3, "count": 0}[mutation]
                    fields[slot] = "999999"
            row[4] = "\t".join(fields)
            Model(text_rows(rows))

# end of layout_observations.py
