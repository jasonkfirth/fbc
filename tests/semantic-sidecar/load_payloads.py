"""Project: FreeBASIC semantic sidecar tests
File: load_payloads.py
Purpose: Prove active payloads of retained and newly generated native loader nodes.
Responsibilities: Class coverage, exact values, call/table/token closure and corruption.
This file intentionally does NOT substitute an earlier snapshot for a load payload.
"""

from driver_inputs import text_rows
from semantic_load_payloads import FEATURE
from sidecar import Model


def check_payloads(test):
    observed = set()
    for backend in test.backends:
        for filename in ("control-flow.bas", "call-abi.bas", "load-actions.bas"):
            with test.subTest(backend=backend, fixture=filename):
                source = test.fixture(filename)
                test.invoke([source], mode="off", backend=backend, extra=("-exx",))
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=("-exx",))
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                state = model.native_load_payloads[1]
                test.assertEqual(set(state["shapes"]), set(model.native_loads[1]["enters"]))
                test.assertEqual(set(state["payloads"]), set(state["shapes"]))
                observed.update(row[0] for row in state["payloads"].values())
                for identity, payload in state["payloads"].items():
                    if payload[0] == "call":
                        test.assertEqual(len(state["arguments"][identity]), int(payload[1]))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
    test.assertTrue({"constant", "call", "operator", "assignment", "offset", "conversion", "index", "sequence"} <= observed, observed)
    assembly = test.source("sub LoadAssembly()\ndim slot as long\nasm\nmov eax, slot\nmov slot, eax\n"
                           "load_assembly_label:\nnop\nend asm\nend sub\n", "load-assembly.bas")
    for backend in test.backends:
        with test.subTest(backend=backend, fixture="assembly"):
            model = test.compile(assembly, backend=backend)
            state = model.native_load_payloads[1]
            entry_tokens = [row for sequence in state["assembly"].values() for row in sequence.values()]
            output = model.native_assembly_outputs[1]
            tokens = [row for sequence in output["tokens"].values() for row in sequence.values()]
            test.assertTrue(entry_tokens)
            test.assertTrue(tokens)
            test.assertEqual(sum(row[0] == "symbol" for row in tokens), 2)
            test.assertIn("load_assembly_label", "".join(row[2] for row in tokens if row[0] == "text").lower())
            # Native C/LLVM loading joins later line streams after entry.
            # Their emptied AST nodes must not invent a second emission.
            test.assertLess(len(entry_tokens), len(tokens))
            test.assertFalse(model.named("load_assembly_label"))


def check_type_abis(test):
    source = test.fixture("call-abi.bas")
    native = test.compile(source, extra=("-semantic-type-abi", "native"))
    stable = test.compile(source, extra=("-semantic-type-abi", "stable-v1"))
    def normalized(model):
        return {identity: (model.primitives[0][int(row[0]) & 31][2] if int(row[0]) != 2147483648 else "invalid",
                           int(row[0]) & ~(31 | (31 << 20)),
                           model.primitives[0][(int(row[0]) >> 20) & 31][2] if (int(row[0]) >> 20) & 31 else "",
                           *row[1:]) for identity, row in model.native_load_payloads[1]["shapes"].items()}
    test.assertEqual(normalized(native), normalized(stable))
    test.assertTrue(any(left[0] != stable.native_load_payloads[1]["shapes"][identity][0]
                        for identity, left in native.native_load_payloads[1]["shapes"].items()))


def check_rejection(test):
    model = test.compile(test.fixture("control-flow.bas"))
    cases = (("native-load-shape:", 0, "4294967295"), ("native-load-shape:", 1, "4294967295"),
             ("native-load-shape:", 4, "2"), ("native-load-payload:", 0, "other-class"),
             ("native-load-argument:", 0, "4294967295"), ("native-load-argument:", 1, "unknown"),
             ("native-load-argument:", 2, "-1"), ("native-load-argument:", 3, "4294967295"),
             ("native-load-argument:", 5, "4294967295"), ("native-load-argument:", 6, "2"),
             ("native-load-payload-counts:", 0, "0"))
    for prefix, slot, value in cases:
        with test.subTest(prefix=prefix, slot=slot):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            fields = row[4].split("\t")
            fields[slot] = value
            row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    for prefix, slot in (("native-load-shape:", 0), ("native-load-payload:", 1), ("native-load-argument:", 2)):
        with test.subTest(recounted=prefix):
            rows = [row.copy() for row in model.rows]
            victim = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            rows.remove(victim)
            closure = next(row for row in rows if row[0] == "K" and row[3].startswith("native-load-payload-counts:"))
            fields = closure[4].split("\t")
            fields[slot] = str(int(fields[slot]) - 1)
            closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    rows = [row.copy() for row in model.rows]
    next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
    with test.assertRaises(ValueError):
        Model(text_rows(rows))


def check_assembly_rejection(test):
    source = test.source("sub EmittedLoadAssembly()\ndim slot as long\nasm\nmov eax, slot\n"
                         "mov slot, eax\nemitted_load_label:\nnop\nend asm\nend sub\n", "emitted-load-assembly.bas")
    model = test.compile(source)
    for mutation in ("owner", "count", "token-symbol", "recounted-token", "unavailable"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def packet(prefix):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            if mutation == "owner":
                row = packet("native-load-assembly-output:")
                fields = row[4].split("\t")
                fields[0] = "1000000"
                row[4] = "\t".join(fields)
            elif mutation == "count":
                row = packet("native-load-assembly-output:")
                fields = row[4].split("\t")
                fields[1] = "0"
                row[4] = "\t".join(fields)
            elif mutation == "token-symbol":
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("native-load-assembly-output-token:") and row[4].split("\t")[0] == "symbol")
                row[4] = "symbol\t0\t"
            elif mutation == "recounted-token":
                tokens = [row for row in rows if row[0] == "K" and row[3].startswith("native-load-assembly-output-token:")]
                rows.remove(tokens[-1])
                closure = packet("native-load-assembly-output-counts:")
                fields = closure[4].split("\t")
                fields[1] = str(int(fields[1]) - 1)
                closure[4] = "\t".join(fields)
            else:
                next(row for row in rows if row[:3] == ["CAP", "1", "native-asm-load-emission"])[3] = "unavailable"
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of load_payloads.py
