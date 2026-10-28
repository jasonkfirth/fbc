"""Project: FreeBASIC semantic sidecar tests
File: contexts.py
Purpose: Prove effective snapshots cannot silently omit options.
Responsibilities: Full option vocabulary, committed contexts and repaired-total rejection.
This file intentionally does NOT derive configuration values from source text.
"""

from driver_inputs import text_rows
from semantic_contexts import REQUIRED
from sidecar import Model


def check_completeness(test):
    source = test.source('#lang "fblite"\noption base 1\ndim values(2) as long\nprint values(1)\n',
                         "context-completeness.bas")
    model = test.compile(source)
    test.assertTrue(model.configurations)
    test.assertEqual(model.capabilities[1]["complete-effective-contexts"], "available")
    context = next(iter(model.configurations))
    # Older schema-27 models did not promise the optional REM keyword key.
    # Explicitly unavailable completeness must retain that compatibility.
    legacy = [row.copy() for row in model.rows]
    for row in legacy:
        if row[0] == "CAP" and row[2] == "complete-effective-contexts":
            row[3] = "unavailable"
    legacy = [row for row in legacy if not (row[0] == "OPT" and row[2:4] == ["language-policy", "remkeyword-active"])]
    restored = Model(text_rows(legacy))
    test.assertTrue(restored.configurations)
    for domain, keys in REQUIRED.items():
        for key in sorted(keys):
            with test.subTest(missing_context_option=(domain, key)):
                rows = [row.copy() for row in model.rows]
                missing = next(row for row in rows if row[:4] == ["OPT", str(context), domain, key])
                rows.remove(missing)
                with test.assertRaisesRegex(ValueError, "configuration snapshot is incomplete"):
                    Model(text_rows(rows))
    for mutation in ("unknown-compiler", "unknown-default", "unknown-policy", "foreign-occurrence"):
        with test.subTest(configuration_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            domain, key = {"unknown-compiler": ("compiler", "guessed-setting"),
                           "unknown-default": ("language-default", "guessed-default"),
                           "unknown-policy": ("language-policy", "guessed-policy"),
                           "foreign-occurrence": ("language-default", "option-statement-999999")}[mutation]
            rows.insert(-1, ["OPT", str(context), domain, key, "0"])
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_modes(test):
    source = test.source('dim value as double\nprint value\n', "context-modes.bas")
    for abi in ("native", "stable-v1"):
        for backend in test.backends:
            with test.subTest(backend=backend, abi=abi):
                extra = ("-semantic-type-abi", abi)
                model = test.compile(source, backend=backend, extra=extra)
                expected = {(domain, key) for domain, keys in REQUIRED.items() for key in keys}
                test.assertTrue(all(expected <= model.options[context].keys() for context in model.configurations))
    for mode in ("expressions", "bindings"):
        model = test.compile(source, mode=mode)
        test.assertFalse(model.configurations)
        test.assertFalse(model.options)

# end of contexts.py
