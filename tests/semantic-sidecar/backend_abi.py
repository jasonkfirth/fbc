"""Project: FreeBASIC semantic sidecar tests
File: backend_abi.py
Purpose: Prove selected backend ABI components and their deliberate limits.
Responsibilities: Real emitted headers, split/indirect adaptations and corrupt closure.
This file intentionally does NOT treat formatted types as machine locations.
"""

import subprocess

from driver_inputs import text_rows
from semantic_backend_abi import FEATURE
from sidecar import Model


def frames(model, identity):
    return {frame: fields for frame, fields in model.selected_backend_abi[1]["signature"].items()
            if fields[0] == str(identity)}


def components(model, frame):
    return [value for value in model.selected_backend_abi[1]["parameter"].values() if value[0] == str(frame)]


def check_choices(test):
    source = test.fixture("backend-abi.bas")
    for backend in test.backends:
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            with test.subTest(backend=backend, target=target):
                extra = ("-target", target)
                test.invoke([source], mode="off", backend=backend, extra=extra)
                emission = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                facts = model.selected_backend_abi[1]
                wide = frames(model, test.one(model, "WideBackend", "procedure"))
                test.assertTrue(wide)
                for frame, signature in wide.items():
                    selected = components(model, frame)
                    if backend == "llvm" and target.endswith("-x86"):
                        test.assertEqual(signature[3], "void")
                        test.assertEqual([value[5] for value in selected], ["hidden-result", "i386-int128-low", "i386-int128-high"])
                        test.assertEqual(selected[0][9], " sret(i128) align 16")
                        test.assertEqual([value[6] for value in selected[1:]], ["i64", "i64"])
                    else:
                        test.assertEqual([value[5] for value in selected], ["value"])
                memory = frames(model, test.one(model, "MemoryBackend", "procedure"))
                for frame in memory:
                    selected = components(model, frame)
                    if backend == "llvm" and target == "linux-x86_64":
                        test.assertEqual(selected[0][5], "memory-byval")
                        test.assertIn(" byval(", selected[0][9])
                    else:
                        test.assertEqual(selected[0][5], "value")
                valist = frames(model, test.one(model, "VaListBackend", "procedure"))
                if backend == "llvm" and target == "linux-x86_64":
                    test.assertTrue(all(components(model, frame)[0][5] == "va-list-decay" for frame in valist))
                if backend != "llvm":
                    register = frames(model, test.one(model, "RegisterBackend", "procedure"))
                    test.assertTrue(any(components(model, frame)[0][5] == "libc-callback" for frame in register))
                    adapted = frames(model, test.one(model, "FreeBackend", "procedure"))
                    test.assertTrue(any(components(model, frame)[0][5] == "libc-adjusted" for frame in adapted))
                unused = test.one(model, "UnusedBackend", "procedure")
                test.assertFalse(frames(model, unused))
                pascal = test.one(model, "PascalBackend", "procedure")
                formals = {int(value[3]): value[2] for value in model.records["G"] if value[1] == str(pascal)}
                for frame in frames(model, pascal):
                    test.assertEqual([value[1] for value in components(model, frame)], [formals[1], formals[0]])
                test.assertTrue(any(value[2] == "pointer-type" for value in facts["signature"].values()))
                emitted = emission.decode("ascii")
                for frame, signature in facts["signature"].items():
                    if signature[2] != "pointer-type":
                        test.assertIn(facts["end"][frame][1], emitted)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_omissions(test):
    source = test.fixture("backend-abi.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                reduced = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(reduced.selected_backend_abi)
    basic = test.source("function SimpleBackend(byval value as long) as long\nreturn value\nend function\n")
    for backend, target in (("gas", "linux-x86"), ("gas64", "linux-x86_64")):
        model = test.compile(basic, backend=backend, extra=("-target", target))
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertFalse(model.selected_backend_abi)


def check_transport(test):
    source = test.fixture("backend-abi.bas")
    text = test.compile(source)
    artifact = test.working / "backend-abi.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(artifact), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(artifact).rows, text.rows)
    first = test.source("sub FirstBackend(byval value as long)\nend sub\n", "first-backend.bas")
    second = test.source("sub SecondBackend(byval value as double)\nend sub\n", "second-backend.bas")
    _, artifact = test.invoke([first, second])
    multiple = Model.read(artifact)
    test.assertEqual(set(multiple.selected_backend_abi), {1, 2})
    test.assertEqual(list(multiple.selected_backend_abi[2]["signature"])[0], 1)


def check_rejection(test):
    model = test.compile(test.fixture("backend-abi.bas"), backend="llvm", extra=("-target", "linux-x86"))
    for mutation in ("stage", "role", "result", "owner", "frame", "position", "part", "kind", "word",
                     "variadic", "count", "missing-word", "duplicate-formal", "end", "closed", "unavailable"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def packet(family, predicate=lambda values: True):
                return next(row for row in rows if row[0] == "K" and row[3].startswith("emitted-abi-" + family + ":")
                            and predicate(row[4].split("\t")))
            if mutation in ("stage", "role", "result", "owner"):
                row = packet("signature")
                values = row[4].split("\t")
                slot = {"stage": 1, "role": 2, "result": 3, "owner": 0}[mutation]
                values[slot] = {"stage": "c", "role": "pretend", "result": "not-the-return-type", "owner": "999999"}[mutation]
            elif mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
                Model(text_rows(rows))
                continue
            elif mutation in ("end", "count", "closed"):
                row = packet("counts" if mutation == "count" else "end")
                values = row[4].split("\t")
                if mutation == "closed":
                    rows.remove(row)
                    Model(text_rows(rows))
                    continue
                values[0] = "999999"
            elif mutation == "duplicate-formal":
                grouped = {}
                for value in rows:
                    if value[0] == "K" and value[3].startswith("emitted-abi-parameter:"):
                        parts = value[4].split("\t")
                        if parts[1] != "0" and parts[4] == "1":
                            grouped.setdefault(parts[0], []).append(value)
                first, second = next(values[:2] for values in grouped.values()
                                     if len(values) > 1 and values[0][4].split("\t")[1] != values[1][4].split("\t")[1])
                values = second[4].split("\t")
                values[1] = first[4].split("\t")[1]
                second[4] = "\t".join(values)
                Model(text_rows(rows))
                continue
            else:
                row = packet("parameter", lambda values: mutation not in ("part", "word", "missing-word")
                             or values[5] == "i386-int128-high")
                values = row[4].split("\t")
                if mutation == "missing-word":
                    rows.remove(row)
                    Model(text_rows(rows))
                    continue
                slot = {"frame": 0, "position": 2, "part": 3, "kind": 5, "word": 6, "variadic": 5}[mutation]
                values[slot] = {"frame": "999999", "position": "999999", "part": "0", "kind": "pretend",
                                "word": "i32", "variadic": "variadic"}[mutation]
            row[4] = "\t".join(values)
            Model(text_rows(rows))

def check_guards(test):
    executable = test.working / "backend-abi-guards"
    sources = ("tooling/semantic-backend-abi.bas", "support/containers/hash.bas",
               "support/containers/list.bas", "support/allocate.bas")
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("backend-abi-guards.bas")),
               *(str(test.root / "src/compiler" / source) for source in sources), "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "BACKEND_ABI_GUARDS_PASS")

# end of backend_abi.py
