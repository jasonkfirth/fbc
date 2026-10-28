"""Project: FreeBASIC semantic sidecar tests
File: intrinsic_dispatch.py
Purpose: Prove native intrinsic selections without promoting attempts to execution.
Responsibilities: Route families, folded roots, fallback, exclusion and transport checks.
This file intentionally does NOT implement BASIC grammar or intrinsic resolution.
"""

from collections import Counter
import subprocess
import os

from driver_inputs import text_rows
from recovery_steps import recovered
from recovery_validity import encoded
from semantic_intrinsics import FEATURE, PREFIX, ROUTES
from sidecar import Model


STATEMENTS = '''#lang "fblite"
option gosub
dim values(0 to 3) as long, x as long, y as long
dim text as string = "abc"
print x
? y
lprint x
data 1, 2
restore
read x
erase values
swap x, y
line input #1, text
input #1, x
poke long, @x, 1
open "example" for binary as #1
seek #1, 1
put #1, , x
get #1, , y
lock #1, 1 to 2
unlock #1, 1 to 2
close #1
name "old" as "new"
on x goto finished, finished
goto finished
finished:
write #1, x
error 5
err = 0
view print
mid(text, 1, 1) = "a"
lset text = "left"
rset text = "right"
width 80
color 7, 0
gosub handler
goto last_statement
handler:
return
resume next
last_statement:
'''
STATEMENT_ROUTES = {
    "cPrintStmt", "cDataStmt", "cEraseStmt", "cSwapStmt", "cLineInputStmt", "cInputStmt",
    "cPokeStmt", "cFileStmt", "cOnStmt", "cGotoStmt", "cWriteStmt", "cErrorStmt",
    "cErrSetStmt", "cViewStmt", "cMidStmt", "cLRSetStmt", "cWidthStmt", "cColorStmt",
}

FUNCTIONS = '''#include once "fbserialize.bi"
dim value as long = 1
dim values(0 to 2) as long
dim text as string = "abc"
print mkd(1), mks(1), mki(1), mkl(1), mkshort(1), mklongint(1)
print cvd(mkd(1)), cvs(mks(1)), cvi(mki(1)), cvl(mkl(1)), cvshort(mkshort(1)), cvlongint(mklongint(1))
print str(value), wstr(value), ustring(text), uchr(65), mid(text, 1), string(2, 65), wstring(2, 65)
print chr(65), wchr(65), asc(text), instr(text, "a"), instrrev(text, "a")
print trim(text), rtrim(text), ltrim(text), lcase(text), ucase(text)
print abs(-1), sgn(-1), fix(1.5), frac(1.5), len(text), sizeof(value)
print sin(1), asin(0), cos(1), acos(0), tan(1), atn(1), sqr(4), log(1), exp(0), atan2(1, 1), int(1.5)
print peek(long, @value), lbound(values), ubound(values), err
print iif(value, 1, 2)
print cbyte(1), cshort(1), cint(1), clng(1), clngint(1)
print cubyte(1), cushort(1), cuint(1), culng(1), culngint(1)
print csng(1), cdbl(1), csign(1u), cunsg(1), cbool(1)
print seek(1), input(1, 1), winput(1, 1), open("example" for binary as #1)
print get(1, , value), put(1, , value), close(1), name("old", "new")
print view print(), width(), color(), screen(1, 1), tcp accept(1)
dim payload as string = serialize(value)
print deserialize(payload, value)
type DispatchValue
    member as long
end type
dim instance as DispatchValue = type<DispatchValue>(1)
sub Worker()
end sub
dim thread as any ptr = threadcall Worker()
'''
FUNCTION_ROUTES = {
    "cMKXFunct", "cCVXFunct", "cStringFunct", "cMathFunct", "cPeekFunct", "cArrayFunct",
    "cErrorFunct", "cStrIdxOrMemberDeref", "cTypeConvExpr", "cFileFunct", "cViewStmt",
    "cWidthStmt", "cColorStmt", "cScreenFunct", "cSerializationFunct", "cThreadCallFunc",
}

GRAPHICS = '''dim image(0 to 200) as ulong
pset (1, 2), 3
preset (1, 2), 3
line (0, 0)-(10, 10), 4, b
circle (5, 5), 2, 7
paint (1, 1), 7
draw "r1"
view (0, 0)-(10, 10)
window (0, 0)-(10, 10)
palette 1, 2
put (0, 0), image, pset
get (0, 0)-(2, 2), image
screen 12
print point(1, 1)
dim buffer as any ptr = imagecreate(4, 4)
'''
GRAPHICS_ROUTES = {
    "cGfxPSet", "cGfxLine", "cGfxCircle", "cGfxPaint", "cGfxDraw", "cGfxView",
    "cGfxPalette", "cGfxPut", "cGfxGet", "cGfxScreen", "cGfxPoint", "cGfxImageCreate",
}

SOUND = '''music stop
sfx stop
midi close
device list
capture stop
print music load("example"), sfx status(1), midi open(0), device select(0), capture status()
'''
SOUND_ROUTES = {"hParseMusic", "hParseSfx", "hParseMidi", "hParseDevice", "hParseCapture"}

VARARGS = '''sub Variadic cdecl(byval first as long, ...)
    dim args as cva_list, copied as cva_list
    cva_start(args, first)
    cva_copy(copied, args)
    print cva_arg(args, long)
    cva_end(args)
    cva_end(copied)
end sub
'''


def check_guards(test):
    executable = test.working / ("intrinsic-guards.exe" if test.native_windows else "intrinsic-guards")
    sources = [test.root / "tests/semantic-sidecar/intrinsic-dispatch-guards.bas",
               test.root / "src/compiler/tooling/semantic-intrinsics.bas"]
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", "intrinsic-dispatch-guards", "-x", str(executable), *map(str, sources)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=60)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("INTRINSIC_DISPATCH_GUARDS_PASS", result.stdout)


def check_routes(test):
    for backend in test.backends:
        covered = {"statement": set(), "function": set()}
        for label, source_text, expected in (
            ("statements", STATEMENTS, STATEMENT_ROUTES),
            ("functions", FUNCTIONS, FUNCTION_ROUTES),
            ("graphics", GRAPHICS, GRAPHICS_ROUTES),
            ("sound", SOUND, SOUND_ROUTES),
            ("varargs", VARARGS, {"cVALISTStmt", "cVALISTFunct"}),
            ("qb", '#lang "qb"\nprint 1: rem comment\nscreen 12\nprint screen(1, 1)\n',
             {"cPrintStmt", "cComment", "cGfxScreen", "cScreenFunct"}),
        ):
            with test.subTest(backend=backend, family=label):
                source = test.source(source_text, "intrinsic-" + label + ".bas")
                test.invoke([source], mode="off", backend=backend)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                observations = model.intrinsic_dispatch[1]
                for step in observations:
                    covered[step[0]].add(step[1])
                selected = {step[1] for step in observations if step[6] == "accepted"}
                test.assertFalse(expected - selected, expected - selected)
                for step in observations:
                    test.assertEqual(step[12], test.compiler_path(source))
                    test.assertIn(step[6], ("accepted", "declined"))
                    test.assertEqual(step[4:6], ["0", "0"])
                    if step[1] == "cComment":
                        test.assertEqual(step[7], "0")
                    else:
                        test.assertIn(int(step[7]), model.statements)
                if label == "graphics":
                    # LINE/VIEW/GET/PUT first try their nongraphics grammar.
                    # A declined route must stay separate from the final selection.
                    test.assertTrue(any(step[1] == "cFileStmt" and step[6] == "declined" for step in observations))
                    test.assertFalse(any(op[2] in ("file-get", "file-put") for op in model.records["SOP"]))
                if label == "functions":
                    test.assertTrue(any(step[1] == "cMathFunct" and step[10] == "16" for step in observations))
                    for route, count in (("cMathFunct", 17), ("cStringFunct", 17), ("cTypeConvExpr", 15),
                                         ("cMKXFunct", 6), ("cCVXFunct", 6), ("cSerializationFunct", 2)):
                        tokens = {step[2] for step in observations if step[1] == route and step[6] == "accepted"}
                        test.assertEqual(len(tokens), count, route)
        source = test.source("sub Variadic cdecl(byval first as long, ...)\nprint va_first()\nend sub\n",
                             "intrinsic-va-first.bas")
        supported = backend not in ("gcc", "clang")
        baseline, _ = test.invoke([source], mode="off", backend=backend, success=supported)
        if supported:
            model = test.compile(source, backend=backend)
        else:
            result, model = recovered(test, source, backend)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        matching = [step for step in model.intrinsic_dispatch[1] if step[1] == "cVAFunct"]
        test.assertEqual(len(matching), 1)
        test.assertEqual(matching[0][6], "accepted" if supported else "recovered")
        covered["function"].add("cVAFunct")
        for kind in ROUTES:
            test.assertEqual(covered[kind], ROUTES[kind], (backend, kind, ROUTES[kind] - covered[kind]))


def check_exclusions(test):
    source = test.source('''#if 0
print abs(-1)
pset (1, 1)
#endif
#assert len("abc") = 3
#define Selected abs(-2)
declare function Lookalike alias "fb_Abs"(byval value as long) as long
dim answer as long = Lookalike(1)
print Selected
''', "intrinsic-exclusions.bas")
    model = test.compile(source)
    steps = model.intrinsic_dispatch[1]
    test.assertEqual(Counter(step[1] for step in steps), {"cMathFunct": 1, "cPrintStmt": 1})
    test.assertTrue(any(step[1] == "cMathFunct" and step[18] == "0" for step in steps))
    test.assertFalse(any(step[14] in ("2", "3", "5") for step in steps))
    # A spelling reused as a user identifier must take its ordinary grammar.
    source = test.source('#lang "qb"\ndim abs%\nabs% = 2\n', "intrinsic-qb-identifier.bas")
    model = test.compile(source)
    test.assertFalse(model.intrinsic_dispatch)
    for label, text, route in (
        ("bad-math", "print abs()\n", "cMathFunct"),
        ("bad-graphics", "pset (1, )\n", "cGfxPSet"),
        ("bad-sound", "dim answer as long = device list\n", "hParseDevice"),
    ):
        with test.subTest(recovery=label):
            source = test.source(text, label + ".bas")
            baseline, _ = test.invoke([source], mode="off", success=False)
            result, model = recovered(test, source)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            matching = [step for step in model.intrinsic_dispatch[1] if step[1] == route]
            test.assertTrue(matching)
            test.assertTrue(all(step[6] == "recovered" and int(step[5]) > int(step[4]) for step in matching))


def check_unformattable(test):
    source = test.source('#lang "qb"\ndim abs as integer\nabs = 2\n', "intrinsic-unresolved-qb.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
            test.assertNotIn("Internal!", baseline.stdout + baseline.stderr)
            for mode in ("full", "bindings", "expressions"):
                result, path = test.invoke([source], mode=mode, backend=backend, success=False)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                if mode != "expressions":
                    test.assertNotIn("\nEND\t", path.read_text())
                    with test.assertRaises(ValueError):
                        Model.read(path, bindings_only=mode == "bindings")
                    continue
                model = Model.read(path, expressions_only=True, allow_recovery=True)
                unavailable = [step for step in model.recovery_steps[1] if step[0] == "unformattable-type"]
                test.assertTrue(unavailable)
                test.assertTrue(all(step[8] == "1" and step[2] == "tooling/semantic-model.bas" for step in unavailable))
                test.assertTrue(all((int(step[10]) & 31) >= 29 for step in unavailable))
                test.assertFalse(any((int(row[12]) & 31) >= 29 for row in model.records["E"]))
                test.assertNotIn("accepted", model.expression_validity.values())
                for corruption in ("absent-root", "replacement-kind"):
                    rows = [row.copy() for row in model.rows]
                    target = next(row for row in rows if row[0] == "K" and row[4].startswith("unformattable-type\t"))
                    fields = target[4].split("\t")
                    if corruption == "absent-root":
                        fields[8:11] = ["0", "0", "2147483648"]
                    else:
                        fields[0] = "retained-prefix"
                    target[4] = "\t".join(fields)
                    with test.assertRaises(ValueError):
                        Model(encoded(rows), expressions_only=True, allow_recovery=True)


def check_transport(test):
    source = test.source("print abs(sin(1))\n", "intrinsic-nested.bas")
    for mode in ("full", "bindings", "expressions"):
        for abi in ("native", "stable-v1"):
            extra = ("-semantic-type-abi", abi)
            model = test.compile(source, mode=mode, extra=extra)
            steps = model.intrinsic_dispatch[1]
            test.assertEqual([step[1] for step in steps], ["cMathFunct", "cMathFunct", "cPrintStmt"])
            test.assertEqual([step[6] for step in steps], ["accepted"] * 3)
            test.assertEqual([step[11] for step in steps[:2]], ["18" if abi == "native" else "16"] * 2)
            path = test.working / (mode + abi + ".fbsem")
            option = {"full": "-semantic-model", "bindings": "-semantic-model-bindings",
                      "expressions": "-semantic-model-expressions"}[mode]
            test.invoke([source], mode="off", extra=(*extra, option, str(path)))
            binary = Model.read(path, bindings_only=mode == "bindings", expressions_only=mode == "expressions")
            test.assertEqual(binary.intrinsic_dispatch, model.intrinsic_dispatch)
    other = test.source("dim value as long\n", "intrinsic-other.bas")
    _, path = test.invoke([source, other])
    model = Model.read(path)
    test.assertEqual(set(model.intrinsic_dispatch), {1})
    counts = [row[4] for row in model.records["K"] if row[3] == PREFIX + "-count"]
    test.assertEqual(counts, ["3", "0"])


def check_source_bytes(test):
    if test.native_windows:
        test.skipTest("Raw non-UTF-8 filenames require a POSIX host")
    source = test.working / os.fsdecode(b"intrinsic-%\t-\xff.bas")
    source.write_bytes(b"print abs(1)\n")
    base = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-r", str(source)]
    baseline = subprocess.run(base, cwd=test.working, capture_output=True, timeout=60)
    test.assertEqual(baseline.returncode, 0, baseline.stdout + baseline.stderr)
    text = test.working / "intrinsic-byte-path.tsv"
    binary = test.working / "intrinsic-byte-path.fbsem"
    models = []
    for path, options in ((text, ["-semantic-model-text"]), (binary, [])):
        result = subprocess.run([*base, *options, "-semantic-model", str(path)],
                                cwd=test.working, capture_output=True, timeout=60)
        test.assertEqual((result.returncode, result.stdout, result.stderr),
                         (baseline.returncode, baseline.stdout, baseline.stderr))
        model = Model.read(path)
        test.assertEqual(len(model.intrinsic_dispatch[1]), 2)
        for step in model.intrinsic_dispatch[1]:
            for index in (12, 19):
                test.assertEqual(os.fsencode(step[index]), os.fsencode(source))
        models.append(model.intrinsic_dispatch)
    test.assertEqual(*models)


def check_runtime(test):
    source = test.source("""dim shared calls as long
function NextValue() as double
    calls += 1
    return calls
end function
dim first as double = abs(sin(NextValue()))
dim skipped as double = iif(0, abs(NextValue()), 0)
print calls
""", "intrinsic-runtime.bas")
    for backend in test.backends:
        baseline = None
        for mode in ("off", "full", "bindings", "expressions"):
            executable = test.working / ("intrinsic-runtime.exe" if test.native_windows else "intrinsic-runtime")
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertEqual(result.stdout.strip(), "1")
            observed = result.stdout, result.stderr
            if baseline is None:
                baseline = observed
            else:
                test.assertEqual(observed, baseline)


def check_rejection(test):
    source = test.source("print abs(1)\n", "intrinsic-corruption.bas")
    model = test.compile(source)
    mutations = ("missing", "summary", "count", "ordinal", "route", "kind", "token", "flag",
                 "outcome", "counter", "statement", "configuration", "class", "root", "source", "point", "duplicate")
    for mutation in mutations:
        with test.subTest(intrinsic_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            step = next(row for row in rows if row[3:4] == [PREFIX + ":1"])
            summary = next(row for row in rows if row[3:4] == [PREFIX + "-count"])
            if mutation in ("missing", "summary"):
                rows.remove(step if mutation == "missing" else summary)
            elif mutation == "count":
                summary[4] = "0"
            elif mutation == "ordinal":
                step[3] = PREFIX + ":2"
            elif mutation == "duplicate":
                rows.insert(-1, step.copy())
            else:
                index, value = {"route": (1, "invented-route"), "kind": (0, "execution"),
                                "token": (2, "-1"), "flag": (3, "2"), "outcome": (6, "declined"),
                                "counter": (4, "1"), "statement": (7, "999999"),
                                "configuration": (8, "999999"), "class": (10, "99"),
                                "root": (9, "0"), "source": (13, "999999"), "point": (17, "-1")}[mutation]
                fields = step[4].split("\t")
                fields[index] = value
                step[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    empty = test.compile(test.source("dim value as long\n", "intrinsic-empty.bas"))
    rows = [row.copy() for row in empty.rows if row[3:4] != [PREFIX + "-count"]]
    with test.assertRaises(ValueError):
        Model(text_rows(rows))

# end of intrinsic_dispatch.py
