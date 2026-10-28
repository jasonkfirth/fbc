"""Project: FreeBASIC semantic sidecar tests
File: resolution.py
Purpose: Exercise actual overload scoring and its deliberate exclusions.
Responsibilities: Success, ambiguity, conversions, defaults, access and native parity.
This file intentionally does NOT resolve candidates from source or infer execution.
"""

import subprocess
import os

from driver_inputs import text_rows
from recovery_steps import recovered
from recovery_validity import encoded
from semantic_resolution import FEATURE, PREFIX
from sidecar import Model


# -------------------------------------------------------------------------
# Native grammar and matching fixtures
# -------------------------------------------------------------------------

NUMERIC = '''declare sub Pick overload(byval value as long)
declare sub Pick overload(byval value as double)
Pick(1l)
'''
DEFAULTS = '''declare sub Defaults overload(byval first as long = 1, byval second as long = 2)
declare sub Defaults overload(byval first as double)
Defaults(1l)
Defaults(, 2l)
declare sub Zero overload(byval value as long = 7)
declare sub Zero overload(byval value as string)
Zero()
declare sub Missing overload(byval first as long, byval second as long)
declare sub Missing overload(byval first as double)
Missing(1.0)
'''
DESCRIPTORS = '''declare sub Matrix overload(values(any, any) as long)
declare sub Matrix overload(values(any) as long)
dim values(0 to 1, 0 to 1) as long
Matrix(values())
declare sub Desc overload(values(any) as long)
declare sub Desc overload(byval value as long)
Desc(1l)
dim value as long
declare sub Ref overload(byref value as const long)
declare sub Ref overload(byref value as long)
Ref(value)
dim fixed as const long = 1
Ref(fixed)
'''
CONVERSIONS = '''type Wrapped
    storage as long
    declare constructor(byval value as long)
end type
declare sub Accept overload(byref value as Wrapped)
declare sub Accept overload(byval value as string)
Accept(1l)
type Converted
    storage as long
    declare operator cast() as long
end type
declare sub AcceptCast overload(byval value as long)
declare sub AcceptCast overload(byval value as string)
dim convertedValue as Converted
AcceptCast(convertedValue)
type NumberValue
    storage as long
    declare operator cast() as long
    declare operator cast() as double
    declare operator cast() as any ptr
end type
dim number as NumberValue
print -number
'''
PROPERTIES = '''type Counter
    storage as long
    declare property Value() as long
    declare property Value(byval updated as long)
end type
dim counterValue as Counter
counterValue.Value = 1
print counterValue.Value
namespace Library
    declare sub Imported overload(byval value as long)
    declare sub Imported overload(byval value as double)
end namespace
using Library
Imported(1l)
type OperationValue
    member as long
end type
declare operator +(byref left as OperationValue, byval right as long) as OperationValue
declare operator +(byref left as OperationValue, byval right as double) as OperationValue
dim leftValue as OperationValue
dim resultValue as OperationValue = leftValue + 1l
'''


# -------------------------------------------------------------------------
# Compiler-produced explanations and their exclusions
# -------------------------------------------------------------------------

def scopes(model, name=None):
    return [scope for scope in model.resolution_scopes.values()
            if name is None or scope["begin"][24].upper() == name.upper()]


def argument_reasons(model):
    return {arg[5] for candidate in model.resolution_candidates.values() for arg in candidate["arguments"]}


def check_guards(test):
    executable = test.working / ("resolution-guards.exe" if test.native_windows else "resolution-guards")
    sources = [test.root / "tests/semantic-sidecar/resolution-guards.bas",
               test.root / "src/compiler/tooling/semantic-resolution.bas"]
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", "resolution-guards", "-x", str(executable), *map(str, sources)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=60)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("RESOLUTION_GUARDS_PASS", result.stdout)


def check_matches(test):
    cases = (("numeric", NUMERIC, {"same-type", "type-difference"}),
             ("defaults", DEFAULTS, {"optional-default", "trailing-optional", "trailing-required"}),
             ("descriptors", DESCRIPTORS, {"descriptor-match", "descriptor-rank", "descriptor-mode",
                                           "compatible-const", "incompatible-const"}),
             ("conversions", CONVERSIONS, {"ctor-conversion", "cast-conversion", "udt-no-conversion"}),
             ("properties", PROPERTIES, set()))
    for backend in test.backends:
        for label, text, expected in cases:
            with test.subTest(backend=backend, matching=label):
                source = test.source(text, "resolution-" + label + ".bas")
                test.invoke([source], mode="off", backend=backend)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                test.assertFalse(expected - argument_reasons(model), expected - argument_reasons(model))
                if label == "numeric":
                    choice = scopes(model, "Pick")[0]
                    test.assertEqual(choice["decision"], "selected")
                    candidates = [model.resolution_candidates[1, identity] for identity in choice["candidates"]]
                    test.assertEqual([candidate["begin"][4] for candidate in candidates], ["screen", "screen", "confirm"])
                    test.assertEqual(candidates[0]["score"][3], "1450")
                    test.assertLess(int(candidates[1]["score"][3]), 1450)
                    test.assertEqual(choice["end"][2], candidates[0]["begin"][2])
                elif label == "defaults":
                    test.assertTrue(any(scope["decision"] == "selected" and scope["end"][4] == "0" for scope in scopes(model, "Zero")))
                    skipped = [candidate for candidate in model.resolution_candidates.values()
                               if candidate["score"][8] == "argument-count"]
                    test.assertTrue(skipped)
                    test.assertTrue(all(candidate["score"][5:7] == ["0", "-1"] for candidate in skipped))
                elif label == "conversions":
                    test.assertTrue(any(scope["begin"][2] != "0" and scope["begin"][3] != "0" for scope in scopes(model)))
                    test.assertTrue(any(scope["begin"][4] == "cast-precision" for scope in scopes(model)))
                    test.assertTrue(any(candidate["score"][8] == "precision-skip" for candidate in model.resolution_candidates.values()))
                elif label == "properties":
                    test.assertTrue(any(candidate["score"][8] == "property-accessor" for candidate in model.resolution_candidates.values()))
                    test.assertTrue(all(scope["begin"][25] == "LIBRARY" for scope in scopes(model, "Imported")))
                    test.assertTrue(any(int(scope["begin"][6]) & 2 for scope in scopes(model)))


def check_rejections(test):
    cases = (
        ("ambiguous", '''declare sub Ambiguous overload(byval first as long, byval second as double)
declare sub Ambiguous overload(byval first as double, byval second as long)
Ambiguous(1l, 1l)
''', "ambiguous"),
        ("no-match", '''type Nominal
    member as long
end type
declare sub NoMatch overload(byval value as string)
declare sub NoMatch overload(byval value as Nominal ptr)
dim value as Nominal
NoMatch(value)
''', "no-match"),
        ("private", '''type Hidden
    storage as long
private:
    declare sub Member overload(byval value as long)
    declare sub Member overload(byval value as double)
end type
dim hiddenValue as Hidden
hiddenValue.Member(1l)
''', "access"),
        ("private-cast", '''type HiddenCast
    storage as long
private:
    declare operator cast() as long
end type
dim hiddenValue as HiddenCast
print clng(hiddenValue)
''', "cast-access"),
    )
    for backend in test.backends:
        for label, text, expected in cases:
            with test.subTest(backend=backend, rejection=label):
                source = test.source(text, "resolution-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, model = recovered(test, source, backend)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                if expected in ("ambiguous", "no-match"):
                    selected = scopes(model, "Ambiguous" if expected == "ambiguous" else "NoMatch")
                    test.assertTrue(selected)
                    test.assertTrue(all(scope["decision"] == expected and scope["end"][2] == "0" and scope["end"][7] == "0"
                                        for scope in selected))
                else:
                    test.assertTrue(any(access[4] == "0" for access in model.resolution_accesses[1]))
                    if expected == "cast-access":
                        test.assertTrue(any(scope["begin"][4] == "cast" and scope["decision"] == "rejected" for scope in scopes(model)))
                test.assertNotIn("accepted", model.expression_validity.values())


def check_exclusions(test):
    source = test.source('''declare sub Pick overload(byval value as long)
declare sub Pick overload(byval value as double)
#if 0
Pick(1l)
#endif
#assert sizeof(long) > 0
declare sub Lookalike alias "PICK"(byval value as long)
Lookalike(1l)
''', "resolution-exclusions.bas")
    model = test.compile(source)
    test.assertFalse(scopes(model, "Pick"))
    test.assertFalse(scopes(model, "Lookalike"))
    for label, text in (
        ("any", "declare sub RejectedAny overload(byref value as any)\n"),
        ("vararg", "declare sub RejectedVararg cdecl overload(byval value as long, ...)\n"),
    ):
        source = test.source(text, "resolution-illegal-" + label + ".bas")
        baseline, _ = test.invoke([source], mode="off", success=False)
        result, rejected = recovered(test, source)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertFalse(any(candidate["begin"][11].startswith("REJECTED") for candidate in rejected.resolution_candidates.values()))
    source = test.source('declare sub DirectVariadic cdecl(byval value as long, ...)\nDirectVariadic(1l, 2l)\n',
                         "resolution-direct-vararg.bas")
    direct = test.compile(source)
    test.assertFalse(scopes(direct, "DirectVariadic"))
    test.assertTrue(any(access[20] == "DIRECTVARIADIC" for access in direct.resolution_accesses[1]))
    source = test.source("dim value as long\n", "resolution-bindings.bas")
    binding = test.compile(source, mode="bindings")
    test.assertEqual(binding.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(binding.resolution_events)


def projection(model):
    result = []
    for scope in scopes(model):
        candidates = [model.resolution_candidates[1, identity] for identity in scope["candidates"]]
        result.append((scope["begin"][4:8], scope["begin"][24:26], scope["decision"],
                       [(candidate["begin"][4:13], candidate["score"][3:], candidate["rank"][3:] if candidate["rank"] else None)
                        for candidate in candidates], scope["end"][2:]))
    return result


def check_transport(test):
    source = test.source(NUMERIC, "resolution-transport.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        models = []
        for mode in ("full", "expressions"):
            model = test.compile(source, mode=mode, extra=extra)
            path = test.working / ("resolution-" + abi + mode + ".fbsem")
            option = "-semantic-model" if mode == "full" else "-semantic-model-expressions"
            test.invoke([source], mode="off", extra=(*extra, option, str(path)))
            binary = Model.read(path, expressions_only=mode == "expressions")
            test.assertEqual(model.resolution_events, binary.resolution_events)
            models.append(model)
        test.assertEqual(projection(models[0]), projection(models[1]))
    other = test.source("dim otherValue as long\n", "resolution-empty.bas")
    _, path = test.invoke([source, other])
    model = Model.read(path)
    test.assertTrue(scopes(model, "Pick"))
    test.assertFalse(any(module == 2 for module, identity in model.resolution_scopes))
    test.assertTrue(any(row[2] == "2" and row[3] == PREFIX + "-counts" for row in model.records["K"]))


def check_corruption(test):
    model = test.compile(test.source(NUMERIC, "resolution-corruption.bas"))
    for mutation in ("missing", "summary", "count", "ordinal", "parent", "kind", "source", "statement",
                     "snapshot", "headless", "parameter-mode", "parameter-score", "uninitialized", "rank", "tie", "winner", "confirm", "duplicate",
                     "policy", "policy-zero", "policy-score", "policy-depth", "policy-duplicate", "access-route", "access-flag"):
        with test.subTest(resolution_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            events = {name: next(row for row in rows if row[3:4] and row[3].startswith(PREFIX + ":")
                                and row[4].startswith(name + "\t")) for name in ("begin", "candidate", "argument", "score", "rank", "end", "access")}
            summary = next(row for row in rows if row[3:4] == [PREFIX + "-counts"])
            policy = next(row for row in rows if row[3:4] == [PREFIX + "-policy"])
            if mutation.startswith("policy"):
                if mutation == "policy":
                    rows.remove(policy)
                elif mutation == "policy-duplicate":
                    rows.insert(-1, policy.copy())
                else:
                    index, value = {"policy-zero": (0, "0"), "policy-score": (1, "1161"), "policy-depth": (3, "1025")}[mutation]
                    fields = policy[4].split("\t"); fields[index] = value; policy[4] = "\t".join(fields)
            elif mutation == "headless":
                fields = events["begin"][4].split("\t")
                fields[18:] = ["0", "0", "2147483648", "0", "0", "-1", "", ""]
                events["begin"][4] = "\t".join(fields)
            elif mutation in ("missing", "summary"):
                rows.remove(events["argument"] if mutation == "missing" else summary)
            elif mutation == "duplicate":
                rows.insert(-1, events["begin"].copy())
            elif mutation == "count":
                fields = summary[4].split("\t"); fields[0] = "0"; summary[4] = "\t".join(fields)
            elif mutation == "ordinal":
                events["begin"][3] = PREFIX + ":2"
            else:
                name, index, value = {"parent": ("begin", 2, "999999"), "kind": ("begin", 4, "hypothetical"),
                                      "source": ("begin", 12, "999999"), "statement": ("begin", 8, "999999"),
                                      "snapshot": ("candidate", 6, "4"), "parameter-mode": ("argument", 7, "99"),
                                      "parameter-score": ("argument", 4, "0"), "uninitialized": ("score", 5, "0"),
                                      "rank": ("rank", 3, "inferior"), "tie": ("rank", 5, "2"),
                                      "winner": ("end", 2, "999999"), "confirm": ("end", 3, "0"),
                                      "access-route": ("access", 3, "hypothetical"), "access-flag": ("access", 4, "2")}[mutation]
                fields = events[name][4].split("\t"); fields[index] = value; events[name][4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_operator_eligibility(test):
    source = test.source("""type LeftValue
    storage as long
end type
type RightValue
    storage as long
    declare operator cast() as long
end type
type ConvertedLeft
    storage as long
    declare constructor(byref value as LeftValue)
end type
declare operator +(byref left as ConvertedLeft, byval right as long) as long
dim lhs as LeftValue, rhs as RightValue
print lhs + rhs
""", "resolution-operator-eligibility.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
        result, model = recovered(test, source, backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        rejected = [candidate for candidate in model.resolution_candidates.values()
                    if candidate["rank"] is not None and candidate["rank"][3] == "ineligible"]
        test.assertTrue(rejected)
        for candidate in rejected:
            test.assertEqual(candidate["score"][4], "1")
            test.assertEqual(candidate["score"][6], "0")
            scope = model.resolution_scopes[1, int(candidate["begin"][1])]
            test.assertTrue(int(scope["begin"][6]) & 2)
            test.assertEqual(scope["end"][7], "0")


def check_parameter_branches(test):
    source = test.source("""declare sub WidthGuard overload(byref value as long)
declare sub WidthGuard overload(byval value as byte)
WidthGuard(byval cbyte(1))
declare sub DescriptorGuard overload(values(any) as long)
declare sub DescriptorGuard overload(byval value as long)
dim values(0 to 1) as long
DescriptorGuard(values())
enum ChoiceKind
    OnlyEntry
end enum
declare sub EnumChoice overload(byval value as ChoiceKind)
declare sub EnumChoice overload(byval value as long)
EnumChoice(1l)
EnumChoice(OnlyEntry)
""", "resolution-parameter-branches.bas")
    for backend in test.backends:
        model = test.compile(source, backend=backend)
        test.assertTrue({"byref-byval-width", "argument-descriptor", "enum-mismatch"} <= argument_reasons(model))
    for label, text, expected in (
        ("omission", "declare sub Omitted overload(byval first as long, byval second as long)\n"
                     "declare sub Omitted overload(byval first as double, byval second as double)\nOmitted(, 1l)\n",
         "required-omission"),
        ("descriptor-type", "declare sub WrongArray overload(values(any) as double)\n"
                            "declare sub WrongArray overload(values(any) as string)\n"
                            "dim values(0 to 1) as long\nWrongArray(values())\n", "descriptor-type"),
    ):
        source = test.source(text, "resolution-rejected-" + label + ".bas")
        baseline, _ = test.invoke([source], mode="off", success=False)
        result, model = recovered(test, source)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertIn(expected, argument_reasons(model))


def check_contexts(test):
    source = test.source(NUMERIC, "resolution-preinclude.bi")
    main = test.source('#lang "fblite"\nPick(1l)\n#line 400 "logical-resolution.bas"\nPick(2l)\n', "resolution-contexts.bas")
    extra = ("-include", str(source))
    for mode in ("full", "expressions"):
        baseline, _ = test.invoke([main], mode="off", extra=extra)
        result, path = test.invoke([main], mode=mode, extra=extra)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        model = Model.read(path, expressions_only=mode == "expressions")
        selected = scopes(model, "Pick")
        test.assertEqual(len(selected), 3)
        test.assertEqual([scope["begin"][1] for scope in selected], ["1", "2", "3"])
        remapped = next(scope for scope in selected if scope["begin"][11] == "logical-resolution.bas")
        test.assertEqual(remapped["begin"][13], model.statements[int(remapped["begin"][8])][13])
        test.assertGreaterEqual(int(remapped["begin"][13]), 400)
        # The lexer still knows the direct physical token under #line.
        # This is an origin point, separate from editable ST/B/E ranges.
        test.assertEqual(remapped["begin"][17], "1")
        test.assertEqual(remapped["begin"][15], "4")
        origin = model.source_contexts[int(remapped["begin"][12])]
        test.assertEqual(origin[7], test.compiler_path(main))


def check_source_bytes(test):
    if test.native_windows:
        test.skipTest("Raw non-UTF-8 filenames require a POSIX host")
    source = test.working / os.fsdecode(b"resolution-%\t-\xff.bas")
    source.write_bytes(NUMERIC.encode("ascii"))
    base = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-r", str(source)]
    baseline = subprocess.run(base, cwd=test.working, capture_output=True, timeout=60)
    test.assertEqual(baseline.returncode, 0, baseline.stdout + baseline.stderr)
    observed = []
    for suffix, options in (("tsv", ["-semantic-model-text"]), ("fbsem", [])):
        path = test.working / ("resolution-byte-path." + suffix)
        result = subprocess.run([*base, *options, "-semantic-model", str(path)], cwd=test.working,
                                capture_output=True, timeout=60)
        test.assertEqual((result.returncode, result.stdout, result.stderr),
                         (baseline.returncode, baseline.stdout, baseline.stderr))
        model = Model.read(path)
        selected = scopes(model, "Pick")
        test.assertEqual(len(selected), 1)
        test.assertEqual(os.fsencode(selected[0]["begin"][11]), os.fsencode(source))
        test.assertTrue(all(os.fsencode(access[7]) == os.fsencode(source) for access in model.resolution_accesses[1]))
        observed.append(model.resolution_events)
    test.assertEqual(*observed)


# -------------------------------------------------------------------------
# Native score-band boundaries and runtime behavior
# -------------------------------------------------------------------------

def check_cast_bands(test):
    original = (test.root / "src/compiler/symbols/symb-proc.bas").read_text()
    macro = original.index("#macro hCheckCastOvlEx")
    start = original.index("\t\t\tif( match >= FB_OVLPROC_TYPEMATCH ) then", macro)
    end = original.index("\n\t\t\treturn fbSemanticModelResolutionArgument", start)
    body = original[start:end]
    source = test.working / "resolution-band-guards.bas"
    source.write_text("""' Project: FreeBASIC native cast ranking tests
' File: resolution-band-guards.bas
' Purpose: Exercise the production score-band transform at its boundaries.
' Responsibilities: Native no-match, conversion, cast and upper-band values.
' This file intentionally does NOT choose an overload.
#include once "core/fbint.bi"
#include once "symbols/symb.bi"
function NativeRemap(byval match as FB_OVLPROC_MATCH_SCORE) as FB_OVLPROC_MATCH_SCORE
""" + body + """
    return match
end function
if NativeRemap(FB_OVLPROC_HALFMATCH) <> FB_OVLPROC_CONVMATCH then end 1
if NativeRemap(FB_OVLPROC_HALFMATCH + 1) <> FB_OVLPROC_CONVMATCH + 1 then end 2
if NativeRemap(FB_OVLPROC_TYPEMATCH - 1) <> FB_OVLPROC_CASTMATCH - 1 then end 3
if NativeRemap(FB_OVLPROC_TYPEMATCH) <> FB_OVLPROC_CASTMATCH then end 4
if NativeRemap(FB_OVLPROC_TYPEMATCH + 1) <> FB_OVLPROC_CASTMATCH + 1 then end 5
if NativeRemap(FB_OVLPROC_FULLMATCH) <> FB_OVLPROC_FULLMATCH - FB_OVLPROC_TYPEMATCH + FB_OVLPROC_CASTMATCH then end 6
if NativeRemap(0) <> FB_OVLPROC_CONVMATCH - OvlMatchScore(FB_DATATYPE_STRUCT, 0) then end 7
print "RESOLUTION_BAND_GUARDS_PASS"
' end of resolution-band-guards.bas
""")
    executable = test.working / ("resolution-bands.exe" if test.native_windows else "resolution-bands")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             str(source), "-x", str(executable)], cwd=test.working,
                            capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("RESOLUTION_BAND_GUARDS_PASS", result.stdout)


def check_cast_runtime(test):
    source = test.source("""dim shared conversions as long
 type CastValue
    storage as long
    declare operator cast() as long
end type
operator CastValue.cast() as long
    conversions += 1
    return 17
end operator
function Accept overload(byval value as long) as long
    return value + 10
end function
function Accept overload(byval value as string) as long
    return -1
end function
dim value as CastValue
print Accept(value)
print conversions
""", "resolution-cast-runtime.bas")
    for backend in test.backends:
        expected = None
        for mode in ("off", "full", "expressions", "bindings"):
            executable = test.working / ("resolution-cast.exe" if test.native_windows else "resolution-cast")
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertEqual([int(value) for value in result.stdout.split()], [27, 1])
            if expected is None:
                expected = result.stdout, result.stderr
            else:
                test.assertEqual((result.stdout, result.stderr), expected)


def check_runtime(test):
    source = test.source('''dim shared calls as long
function Tick() as long
    calls += 1
    return calls
end function
function Pick overload(byval value as long) as long
    return value + 10
end function
function Pick overload(byval value as double) as long
    return value + 20
end function
print Pick(Tick())
if 0 then print Pick(Tick())
print calls
''', "resolution-runtime.bas")
    for backend in test.backends:
        baseline = None
        for mode in ("off", "full", "expressions", "bindings"):
            executable = test.working / ("resolution-runtime.exe" if test.native_windows else "resolution-runtime")
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertEqual([int(value) for value in result.stdout.split()], [11, 1])
            if baseline is None:
                baseline = result.stdout, result.stderr
            else:
                test.assertEqual((result.stdout, result.stderr), baseline)

# end of resolution.py
