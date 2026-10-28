"""Project: FreeBASIC semantic sidecar tests
File: consumed_tokens.py
Purpose: Verify committed native token payloads and their exclusions.
Responsibilities: Active unions, raw/wire type distinction, provenance and corruption.
This file intentionally does NOT infer lexer tokens from source text.
"""

import subprocess

from driver_inputs import text_rows
from sidecar import Model
from semantic_tokens import FEATURE

SOURCE = '''#lang "fblite"
#define ARG(x) x
#if 0
InactiveName = 999
#endif
namespace Area
dim shared MemberValue as double
end namespace
dim MixedCase% = 12
dim NumberValue as double = ARG(0001.25e+2#)
Area.MemberValue = NumberValue + MixedCase%
print !"A\\nB", Area.MemberValue
'''


def check_guards(test):
    source = test.working / "consumed-token-guards.bas"
    source.write_bytes((test.root / "tests/semantic-sidecar/consumed-token-guards.bas").read_bytes())
    collector = test.working / "semantic-tokens.bas"
    collector.write_bytes((test.root / "src/compiler/tooling/semantic-tokens.bas").read_bytes())
    executable = test.working / ("consumed-token-guards.exe" if test.native_windows else "consumed-token-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", source.stem, "-x", str(executable), str(source), str(collector)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=300)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("CONSUMED_TOKEN_GUARDS_PASS", result.stdout)


def check_payloads(test):
    source = test.source(SOURCE, "consumed-token-payloads.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        tokens = model.consumed_tokens[1]
        test.assertTrue({0, 1, 2, 3, 4, 5, 6} <= {int(row[1]) for row in tokens})
        test.assertTrue(any(row[7] == "MixedCase" and row[28] == "1" and row[29] == str(ord('%')) for row in tokens))
        test.assertTrue(any(row[1] == "3" and row[30] == "1" and row[28] == "0" for row in tokens))
        test.assertTrue(any(row[26] == "string-escapes-present" and row[27] == "1" for row in tokens))
        test.assertTrue(any(row[1] == "5" and row[3:5] == ["0", "2147483648"] for row in tokens))
        test.assertFalse(any(row[7] == "InactiveName" for row in tokens))
        test.assertTrue(any(row[18] != "0" for row in tokens))
        test.assertTrue(any(row[42] != "0" and row[7] == "MixedCase" for row in tokens))


def check_wide(test):
    source = test.working / "consumed-token-wide.bas"
    source.write_bytes(b'\xff\xfe' + 'const TextValue = "\u03a9"\nprint TextValue\n'.encode('utf-16-le'))
    baseline, _ = test.invoke([source], mode="off")
    result, path = test.invoke([source])
    test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
    model = Model.read(path)
    wide = [row for row in model.consumed_tokens[1] if row[6] == "wide-units"]
    test.assertTrue(wide)
    test.assertTrue(any("000003A9" in row[7] for row in wide))


def check_modes(test):
    source = test.source(SOURCE, "consumed-token-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        model = test.compile(source, extra=extra)
        path = test.working / ("consumed-token-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(path)))
        binary = Model.read(path)
        test.assertEqual(binary.consumed_tokens, model.consumed_tokens)
        # Raw lexer dtype remains native. Only proven datatype slots have a
        # second value under the selected semantic datatype ABI.
        doubles = [row for row in model.consumed_tokens[1] if row[1] == "3" and row[2] == "18"]
        test.assertTrue(doubles)
        test.assertTrue(all(row[4] == ("18" if abi == "native" else "16") for row in doubles))
    for mode in ("bindings", "expressions"):
        model = test.compile(source, mode=mode)
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertFalse(model.consumed_tokens)
    _, path = test.invoke([source, source])
    model = Model.read(path)
    test.assertEqual(set(model.consumed_tokens), {1, 2})


def check_exclusions(test):
    source = test.source('''#lang "fblite"
#if 0
NeverConsumed = 1
#endif
#if typeof(123) = integer
const SelectedValue = 2
#endif
print SelectedValue
''', "consumed-token-exclusions.bas")
    model = test.compile(source)
    tokens = model.consumed_tokens[1]
    test.assertFalse(any(row[7] == "NeverConsumed" for row in tokens))
    test.assertFalse(any(row[7] == "123" for row in tokens))
    source = test.source('''dim value as long
dim copied as typeof(value)
print copied
''', "consumed-token-lookahead.bas")
    baseline, _ = test.invoke([source], mode="off")
    result, path = test.invoke([source])
    test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
    model = Model.read(path)
    tokens = model.consumed_tokens[1]
    # The detached lookahead probe must not duplicate parser consumption.
    test.assertEqual(sum(row[7] == "value" for row in tokens), 2)


def check_hidden_macros(test):
    source = test.source(SOURCE, "consumed-token-hidden-macros.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend, extra=("-semantic-model-compact",))
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        test.assertFalse(model.macro_invocations)
        test.assertTrue(model.consumed_tokens[1])
        test.assertTrue(all(row[18:21] == ["0", "0", "0"] for row in model.consumed_tokens[1]))
        # Hidden provenance is an intentional graph omission, not permission
        # for a token to point at an unexported internal allocation identity.
        rows = [row.copy() for row in model.rows]
        event = next(row for row in rows if row[3:4] and row[3].startswith("consumed-token:"))
        fields = event[4].split("\t")
        fields[19:21] = ["1", "1"]
        event[4] = "\t".join(fields)
        with test.assertRaises(ValueError):
            Model(text_rows(rows))


def check_corruption(test):
    model = test.compile(test.source(SOURCE, "consumed-token-corruption.bas"))
    for mutation in ("missing", "summary", "policy", "ordinal", "class", "context", "source", "union", "type-domain", "payload", "suffix", "numeric", "lookup", "text-length", "mapped-dtype", "duplicate"):
        with test.subTest(consumed_token_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith("consumed-token:"))
            summary = next(row for row in rows if row[3:4] == ["consumed-token-count"])
            policy = next(row for row in rows if row[3:4] == ["consumed-token-policy"])
            if mutation in ("missing", "summary", "policy"):
                rows.remove({"missing": row, "summary": summary, "policy": policy}[mutation])
            elif mutation == "ordinal":
                row[3] = "consumed-token:999999"
            elif mutation == "duplicate":
                rows.insert(-1, row.copy())
            elif mutation == "mapped-dtype":
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("consumed-token:")
                           and row[4].split("\t")[1] == "3")
                fields = row[4].split("\t")
                fields[4] = "2147483648"
                row[4] = "\t".join(fields)
            else:
                index, value = {"class": (1, "99"), "context": (10, "999999"), "source": (11, "999999"),
                                "union": (26, "guessed"), "type-domain": (3, "9"), "payload": (6, "guessed"),
                                "suffix": (29, "999999"), "numeric": (30, "9"), "lookup": (42, "999999"),
                                "text-length": (5, "999999")}[mutation]
                fields = row[4].split("\t")
                fields[index] = value
                row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

    source = test.source(SOURCE, "consumed-token-foreign-macro.bas")
    _, path = test.invoke([source, source])
    repeated = Model.read(path)
    foreign = next(identity for identity, invocation in repeated.macro_invocations.items()
                   if int(repeated.source_contexts[int(invocation[4])][4]) == 2)
    rows = [row.copy() for row in repeated.rows]
    row = next(row for row in rows if row[0] == "K" and row[1:3] == ["module", "1"]
               and row[3].startswith("consumed-token:") and row[4].split("\t")[18] != "0")
    fields = row[4].split("\t")
    fields[18] = str(foreign)
    row[4] = "\t".join(fields)
    with test.assertRaisesRegex(ValueError, "module-owned macro expansion"):
        Model(text_rows(rows))

# end of consumed_tokens.py
