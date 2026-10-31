"""Project: FreeBASIC semantic sidecar tests
File: call_abi.py
Purpose: Prove selected backend call components retain actual native choices.
Responsibilities: Target partitions, source identities, transport and corruption.
This file intentionally does NOT reconstruct ABI lowering or predict registers.
"""

import subprocess

from driver_inputs import text_rows
from semantic_call_abi import BUILTIN_FEATURE, FEATURE, POINTER_MASK
from sidecar import Model


def calls(model, proc, module=1):
    return {identity: fields for identity, fields in model.selected_call_abi[module]["call"].items()
            if fields[0] == str(proc)}


def arguments(model, frame, module=1):
    return [fields for fields in model.selected_call_abi[module]["argument"].values() if fields[0] == str(frame)]


def check_choices(test):
    source = test.fixture("call-abi.bas")
    for backend in test.backends:
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            with test.subTest(backend=backend, target=target):
                extra = ("-target", target)
                test.invoke([source], mode="off", backend=backend, extra=extra)
                emission = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                facts = model.selected_call_abi[1]
                wide = calls(model, test.one(model, "CallWide", "procedure"))
                test.assertEqual(len(wide), 1)
                frame = next(iter(wide))
                selected = arguments(model, frame)
                if backend == "llvm" and target.endswith("-x86"):
                    test.assertEqual([row[5] for row in selected], ["hidden-result", "i386-int128-low", "i386-int128-high"])
                    test.assertEqual(selected[0][11], " sret(i128) align 16")
                    test.assertEqual(wide[frame][11], "void")
                    test.assertEqual(selected[1][6:8], selected[2][6:8])
                else:
                    test.assertEqual([row[5] for row in selected], ["value"])
                memory = calls(model, test.one(model, "CallMemory", "procedure"))
                selected = arguments(model, next(iter(memory)))
                test.assertEqual(selected[0][5], "memory-byval" if backend == "llvm" and target == "linux-x86_64" else "value")
                valist = calls(model, test.one(model, "CallVaList", "procedure"))
                selected = arguments(model, next(iter(valist)))
                test.assertEqual(selected[0][5], "va-list-decay" if backend == "llvm" and target == "linux-x86_64" else "value")
                variadic = calls(model, test.one(model, "CallVariadic", "procedure"))
                selected = arguments(model, next(iter(variadic)))
                test.assertEqual([row[5] for row in selected], ["value", "variadic-value", "variadic-value", "variadic-value"])
                test.assertEqual([model.primitives[0][int(row[8]) & 31][2] for row in selected[1:]], ["long", "double", "long"])
                optional = calls(model, test.one(model, "CallOptional", "procedure"))
                test.assertEqual(len(optional), 2)
                test.assertEqual(len({row[5] for row in optional.values()}), 2)
                test.assertTrue(all(row[5] != "0" for row in optional.values()))
                test.assertTrue(all(arguments(model, identity)[0][12].strip("()").endswith("17") for identity in optional))
                pascal = test.one(model, "CallPascal", "procedure")
                formal_order = {int(row[3]): row[2] for row in model.records["G"] if row[1] == str(pascal)}
                selected = arguments(model, next(iter(calls(model, pascal))))
                test.assertEqual([row[1] for row in selected], [formal_order[1], formal_order[0]])
                modes = arguments(model, next(iter(calls(model, test.one(model, "CallModes", "procedure")))))
                test.assertEqual(len(modes), 3)
                test.assertTrue(all(int(row[8]) & POINTER_MASK for row in modes))
                test.assertTrue(any(row[2] == "indirect" and row[7] != "0" for row in facts["call"].values()))
                test.assertTrue(any(row[2] == "indirect" and row[7] == "0" for row in facts["call"].values()))
                test.assertTrue(all(row[2] == "direct" for row in wide.values()))
                test.assertTrue(any(row[3] != "0" for row in facts["call"].values()))
                test.assertFalse(calls(model, test.one(model, "CallUnused", "procedure")))
                if backend != "llvm":
                    adapted = arguments(model, next(iter(calls(model, test.one(model, "CallFree", "procedure")))))
                    test.assertEqual(adapted[0][5], "libc-adjusted")
                emitted = emission.decode("ascii")
                for count, text in facts["end"].values():
                    test.assertIn(text, emitted)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_graphics_choices(test):
    source = test.fixture("graphics-call-abi.bas")
    for backend in (value for value in test.backends if value in ("gcc", "clang")):
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            with test.subTest(backend=backend, target=target):
                extra = ("-target", target)
                test.invoke([source], mode="off", backend=backend, extra=extra)
                emission = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                selected = model.selected_call_abi[1]["argument"].values()
                callbacks = [row for row in selected if row[5] == "graphics-blender"]
                contexts = [row for row in selected if row[5] == "graphics-blender-context"]
                test.assertEqual(len(callbacks), 2)
                test.assertEqual(len(contexts), 1)
                test.assertTrue(all(int(row[8]) & POINTER_MASK for row in callbacks))
                test.assertEqual(contexts[0][8:10], ("", ""))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
                for mutation in ("callback-scalar", "callback-subtype", "context-type", "context-kind"):
                    with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                        rows = [row.copy() for row in model.rows]
                        kind = "graphics-blender" if mutation.startswith("callback-") else "graphics-blender-context"
                        row = next(row for row in rows if row[0] == "K"
                                   and row[3].startswith("emitted-abi-argument:")
                                   and row[4].split("\t")[5] == kind)
                        fields = row[4].split("\t")
                        if mutation == "callback-scalar":
                            fields[8] = str(int(fields[8]) & ~POINTER_MASK)
                        elif mutation == "callback-subtype":
                            fields[9] = "0"
                        elif mutation == "context-type":
                            fields[8:10] = fields[6:8]
                        else:
                            fields[5] = "graphics-blender"
                        row[4] = "\t".join(fields)
                        Model(text_rows(rows))


def check_omissions(test):
    source = test.fixture("call-abi.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(model.selected_call_abi)
    for backend, target in (("gas", "linux-x86"), ("gas64", "linux-x86_64")):
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend, extra=("-target", target))
            test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
            test.assertFalse(model.selected_call_abi)


def check_transport(test):
    source = test.fixture("call-abi.bas")
    model = test.compile(source)
    artifact = test.working / "call-abi.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(artifact), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(artifact).rows, model.rows)
    first = test.source("sub FirstCall(byval value as long)\nend sub\nFirstCall(1)\n", "first-call.bas")
    second = test.source("sub SecondCall(byval value as double)\nend sub\nSecondCall(2)\n", "second-call.bas")
    _, artifact = test.invoke([first, second])
    multiple = Model.read(artifact)
    test.assertEqual(set(multiple.selected_call_abi), {1, 2})
    test.assertEqual(next(iter(multiple.selected_call_abi[2]["call"])), 1)
    test.assertEqual(next(iter(multiple.selected_call_abi[2]["argument"])), 1)


def check_rejection(test):
    model = test.compile(test.fixture("call-abi.bas"), backend="llvm", extra=("-target", "linux-x86"))
    mutations = ("owner", "stage", "dispatch", "caller", "expression", "statement", "source", "static-target", "demanded",
                 "result-type", "level", "pop", "call", "position", "part", "kind", "input-type", "selected-type", "value",
                 "variadic", "missing-word", "header", "end-count", "count", "closed", "unavailable")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            if mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
                Model(text_rows(rows))
                continue
            if mutation in mutations[:12]:
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("emitted-abi-call:"))
                fields = row[4].split("\t")
                slot = {"owner": 0, "stage": 1, "dispatch": 2, "caller": 3, "expression": 4, "statement": 5,
                        "source": 6, "static-target": 7, "demanded": 8, "result-type": 9, "level": 13, "pop": 14}[mutation]
                fields[slot] = {"stage": "c", "dispatch": "pretend", "demanded": "2", "result-type": "2147483648",
                                "level": "-1", "pop": "-1"}.get(mutation, "999999")
            elif mutation in ("header", "end-count", "closed", "count"):
                prefix = "emitted-abi-call-counts:" if mutation == "count" else "emitted-abi-call-end:"
                row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
                fields = row[4].split("\t")
                if mutation == "closed":
                    rows.remove(row)
                    Model(text_rows(rows))
                    continue
                fields[1 if mutation == "header" else 0] = "pretend" if mutation == "header" else "999999"
            else:
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("emitted-abi-argument:")
                           and (mutation not in ("part", "missing-word") or row[4].split("\t")[5] == "i386-int128-high")
                           and (mutation != "variadic" or row[4].split("\t")[5] == "variadic-value"))
                fields = row[4].split("\t")
                if mutation == "missing-word":
                    rows.remove(row)
                    Model(text_rows(rows))
                    continue
                slot = {"call": 0, "position": 2, "part": 3, "kind": 5, "input-type": 6,
                        "selected-type": 8, "value": 12, "variadic": 5}[mutation]
                fields[slot] = {"part": "0", "kind": "pretend", "input-type": "2147483648",
                                "selected-type": "2147483648", "value": "not-the-selected-value", "variadic": "value"}.get(mutation, "999999")
            row[4] = "\t".join(fields)
            Model(text_rows(rows))
    for mutation in ("false-indirect", "nonvirtual-static-target"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith("emitted-abi-call:")
                       and (row[4].split("\t")[2] == "direct" if mutation == "false-indirect" else row[4].split("\t")[7] != "0"))
            fields = row[4].split("\t")
            if mutation == "false-indirect":
                fields[2] = "indirect"
            else:
                fields[7] = str(test.one(model, "CallWide", "procedure"))
            row[4] = "\t".join(fields)
            Model(text_rows(rows))

def check_guards(test):
    executable = test.working / "call-abi-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("call-abi-guards.bas")), str(test.root / "src/compiler/tooling/semantic-call-abi.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "CALL_ABI_GUARDS_PASS")


def check_execution(test):
    source = test.fixture("call-abi.bas")
    outputs = []
    for mode in ("off", "full", "bindings", "expressions"):
        with test.subTest(mode=mode):
            executable = test.working / ("call-abi-" + mode)
            test.invoke([source], mode=mode, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertEqual(result.stdout.strip(), "CALL_ABI_PASS")
            outputs.append(result.stdout)
    test.assertEqual(len(set(outputs)), 1)


def check_builtins(test):
    source = test.source('declare function SwapCall cdecl alias "__builtin_bswap32" (byval value as ulong) as ulong\n'
                         'dim value as ulong = SwapCall(&h11223344)\n', "builtin-call.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            proc = test.one(model, "SwapCall", "procedure")
            builtins = model.selected_call_abi[1]["builtin"]
            if backend == "llvm":
                test.assertEqual(model.capabilities[1][BUILTIN_FEATURE], "available")
                test.assertTrue(any(row[0] == str(proc) and row[1] == "llvm-builtin-lowering" for row in builtins.values()))
                test.assertFalse(calls(model, proc))
                test.assertIn(b"llvm.bswap.i32", emission)
                for mutation in ("target", "reason", "caller", "expression", "statement", "source", "static-target",
                                 "level", "count", "missing", "unavailable"):
                    with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                        rows = [row.copy() for row in model.rows]
                        if mutation == "unavailable":
                            next(row for row in rows if row[:3] == ["CAP", "1", BUILTIN_FEATURE])[3] = "unavailable"
                        else:
                            prefix = "emitted-abi-builtin-count:" if mutation == "count" else "emitted-abi-builtin:"
                            row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
                            if mutation == "missing":
                                rows.remove(row)
                            else:
                                fields = row[4].split("\t")
                                slot = {"target": 0, "reason": 1, "caller": 2, "expression": 3, "statement": 4,
                                        "source": 5, "static-target": 6, "level": 7, "count": 0}[mutation]
                                fields[slot] = {"reason": "pretend", "static-target": str(proc), "level": "-1"}.get(mutation, "999999")
                                row[4] = "\t".join(fields)
                        Model(text_rows(rows))
            else:
                test.assertEqual(model.capabilities[1][BUILTIN_FEATURE], "unavailable")
                test.assertFalse(builtins)
                test.assertEqual(len(calls(model, proc)), 1)
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_instrumentation(test):
    source = test.fixture("call-abi.bas")
    for backend in test.backends:
        for extra in (("-exx",), ("-O", "2"), ("-profile",)):
            with test.subTest(backend=backend, options=extra):
                test.invoke([source], mode="off", backend=backend, extra=extra)
                emission = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                test.assertTrue(model.selected_call_abi[1]["call"])
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)

# end of call_abi.py
