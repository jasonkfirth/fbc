"""Project: FreeBASIC semantic sidecar tests
File: emission_names.py
Purpose: Prove selected backend tokens and preserve unknown object identities.
Responsibilities: Aliases, C++ linkage, builtin mappings, target ABI and corruption.
This file intentionally does NOT resolve libraries or infer symbols from source names.
"""

import subprocess
import shutil

from driver_inputs import text_rows
from semantic_emission_names import FEATURE, PREFIX, CLOSURE
from sidecar import Model


def source_fixture(test):
    original = (test.root / "tests/semantic-sidecar/procedure-linkage.bas").read_text()
    return test.source(original + '''
extern "c"
extern foreign_data alias "external_data" as long
thread local extern foreign_tls alias "foreign_tls" as long
end extern
dim shared public_data alias "public_data" as long
private sub LocalWork()
    static retained as long
    dim local_value as long
    retained += foreign_data
    local_value = retained
    print local_value
end sub
LocalWork()
print BoundC(1), BoundCpp(2), AliasedBasic(3), public_data, foreign_tls
''')


def names_for(model, name):
    wanted = {identity for identity in model.named(name)}
    return {stage: (state, spelling, reason)
            for events in model.emission_names.values()
            for owner, stage, state, spelling, reason in events if owner in wanted}


def check_names(test):
    source = source_fixture(test)
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertEqual(names_for(model, "BOUNDC")["object"], ("selected", "BoundC", ""))
            test.assertEqual(names_for(model, "ALIASEDBASIC")["object"], ("selected", "c_style_name", ""))
            test.assertEqual(names_for(model, "BOUNDCPP")["object"], ("selected", "_Z8BoundCppiz", ""))
            test.assertEqual(names_for(model, "FOREIGN_DATA")["object"], ("selected", "external_data", ""))
            test.assertEqual(names_for(model, "PUBLIC_DATA")["object"], ("selected", "public_data", ""))
            test.assertEqual(names_for(model, "FOREIGN_TLS")["object"], ("unavailable", "", "backend-tls"))
            test.assertFalse(names_for(model, "PROTOTYPEONLY"))
            if backend in ("gcc", "clang"):
                test.assertEqual(names_for(model, "RETAINED")["object"], ("unavailable", "", "c-local-static"))
                test.assertEqual(names_for(model, "LOCAL_VALUE")["object"], ("none", "", "automatic-storage"))
            elif backend == "llvm":
                test.assertEqual(names_for(model, "RETAINED")["object"], ("unavailable", "", "llvm-private"))
                test.assertEqual(names_for(model, "LOCALWORK")["object"], ("unavailable", "", "llvm-private"))
                test.assertEqual(names_for(model, "LOCAL_VALUE")["object"], ("none", "", "automatic-storage"))
            stage = "c" if backend in ("gcc", "clang") else backend
            for owner, observed, state, spelling, _ in model.emission_names[1]:
                if observed == stage and state == "selected":
                    test.assertIn(spelling.encode("utf-8", "surrogateescape"), baseline, model.symbols[owner][2])
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mode in ("bindings", "expressions"):
                reduced = test.compile(source, mode=mode, backend=backend)
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(reduced.emission_names)
                test.assertFalse(any(row[3].startswith("emission-name") for row in reduced.records["K"]))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_builtins(test):
    source = test.source('extern "c"\n'
                         'declare function CopyBytes cdecl alias "__builtin_memcpy" '
                         '(byval dst as any ptr, byval src as const any ptr, byval count as uinteger) as any ptr\n'
                         'end extern\n'
                         'dim a(0 to 7) as ubyte, b(0 to 7) as ubyte\n'
                         'CopyBytes(@a(0), @b(0), 8)\n'
                         'dim callback as function cdecl(byval as any ptr, byval as const any ptr, byval as uinteger) as any ptr\n'
                         'callback = @CopyBytes\nprint callback(@a(0), @b(0), 1)\n')
    for backend in test.backends:
        if backend not in ("gcc", "clang", "llvm"):
            continue
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            facts = names_for(model, "CopyBytes")
            expected = (("unavailable", "", "backend-intrinsic") if backend == "gcc"
                        else ("selected", "memcpy", ""))
            test.assertEqual(facts["object"], expected)
            stage = "llvm" if backend == "llvm" else "c"
            if backend == "clang":
                # Clang gets an ordinary C identifier with an explicit libc
                # asm alias. Inspect that emitted declaration, not its name
                # allocator's private counter.
                identifier = facts[stage][1]
                test.assertEqual(facts[stage][0], "selected")
                test.assertTrue(any(identifier in line and '"memcpy"' in line and "__asm__" in line
                                    for line in baseline.decode().splitlines()))
            else:
                test.assertEqual(facts[stage], ("selected", "@memcpy" if backend == "llvm" else "__builtin_memcpy", ""))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            pointer_types = {identity for identity, signature in model.signatures.items() if signature[2] == "procedure-pointer"}
            test.assertTrue(pointer_types)
            test.assertFalse(any(owner in pointer_types for owner, *_ in model.emission_names[1]))


def check_transport_and_objects(test):
    source = source_fixture(test)
    text = test.compile(source)
    binary = test.working / "names.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(binary), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(binary).rows, text.rows)
    executable_object = source.with_suffix(".o")
    # Retain the intermediate with -R so native object bytes do not differ
    # because each invocation chose a different temporary C filename.
    test.invoke([source], mode="off", emit=False, extra=("-c", "-R", "-o", str(executable_object)))
    baseline = executable_object.read_bytes()
    _, artifact = test.invoke([source], mode="full", emit=False, extra=("-c", "-R", "-o", str(executable_object)))
    model = Model.read(artifact)
    test.assertEqual(executable_object.read_bytes(), baseline)
    result = subprocess.run(["nm", "-g", str(executable_object)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    object_symbols = {line.split()[-1] for line in result.stdout.splitlines() if line.split()}
    for name in ("BoundC", "BoundCpp", "AliasedBasic", "foreign_data", "public_data"):
        state, spelling, _ = names_for(model, name)["object"]
        test.assertEqual(state, "selected")
        test.assertIn(spelling, object_symbols)
    secondary = test.source('sub SecondaryAlias alias "secondary_native"()\nend sub\nSecondaryAlias()\n', "secondary.bas")
    _, artifact = test.invoke([source, secondary])
    multiple = Model.read(artifact)
    test.assertEqual(set(multiple.emission_names), {1, 2})
    test.assertTrue(all(multiple.symbols[owner][2].upper() != "SECONDARYALIAS" for owner, *_ in multiple.emission_names[1]))
    test.assertEqual(names_for(multiple, "SecondaryAlias")["object"], ("selected", "secondary_native", ""))


def check_rejection(test):
    model = test.compile(source_fixture(test))
    selected = next(row for row in model.records["K"] if row[3].startswith(PREFIX)
                    and row[4].split("\t")[:2] == ["object", "selected"])
    for mutation in ("missing", "count", "huge-count", "ordinal", "domain", "owner", "stage", "empty",
                     "nul", "unknown-with-name", "reason", "none-procedure", "duplicate-stage", "capability"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            row = next(item for item in rows if item == selected)
            if mutation == "missing":
                rows.remove(row)
            elif mutation in ("count", "huge-count"):
                next(item for item in rows if item[0] == "K" and item[3] == CLOSURE + "1")[4] = "0" if mutation == "count" else "1000001"
            elif mutation == "ordinal":
                row[3] = PREFIX + "999999"
            elif mutation == "domain":
                row[1] = "node"
            elif mutation == "owner":
                row[2] = str(test.one(model, "PROTOTYPEONLY", "procedure"))
                row[4] = "object\tnone\t\tautomatic-storage"
            elif mutation == "capability":
                next(item for item in rows if item[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            elif mutation == "duplicate-stage":
                copy = row.copy()
                count = next(item for item in rows if item[0] == "K" and item[3] == CLOSURE + "1")
                copy[3] = PREFIX + str(int(count[4]) + 1)
                count[4] = str(int(count[4]) + 1)
                rows.insert(rows.index(count), copy)
            else:
                fields = row[4].split("\t")
                if mutation == "stage": fields[0] = "llvm"
                elif mutation == "empty": fields[2] = ""
                elif mutation == "nul": fields[2] = "%00"
                elif mutation == "unknown-with-name": fields[1], fields[3] = "unavailable", "backend-intrinsic"
                elif mutation == "reason": fields[3] = "source-alias"
                else: fields[1:] = ["none", "", "automatic-storage"]
                row[4] = "\t".join(fields)
            Model(text_rows(rows))


def check_target_decoration(test):
    source = test.source('sub NativeStd stdcall alias "native_std"(byval value as long)\nend sub\n'
                         'sub NativeFast __fastcall alias "native_fast"(byval a as long, byval b as long)\nend sub\n'
                         'extern "c++"\ndeclare function CppValue(byval value as long) as long\nend extern\n'
                         'function CppValue cdecl(byval value as long) as long\nreturn value\nend function\n'
                         'extern "c"\nextern foreign_data alias "external_data" as long\nend extern\n'
                         'dim shared public_data alias "public_data" as long\n'
                         'NativeStd(1)\nNativeFast(1, 2)\npublic_data = foreign_data + CppValue(3)\n')
    for target in ("win32-x86", "darwin-x86_64", "linux-x86"):
        expected = {"NATIVESTD": "_native_std@4" if target == "win32-x86" else
                    "_native_std" if target.startswith("darwin") else "native_std",
                    "NATIVEFAST": "@native_fast@8" if target == "win32-x86" else
                    "_native_fast" if target.startswith("darwin") else "native_fast",
                    "FOREIGN_DATA": "external_data" if target.startswith("linux") else "_external_data",
                    "PUBLIC_DATA": "public_data" if target.startswith("linux") else "_public_data",
                    "CPPVALUE": "_Z8CppValuei" if target.startswith("linux") else "__Z8CppValuei"}
        for backend in test.backends:
            if backend == "gas64" or backend == "gas" and target != "linux-x86":
                continue
            with test.subTest(target=target, backend=backend):
                extra = ("-target", target)
                test.invoke([source], mode="off", backend=backend, extra=extra)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                for name, spelling in expected.items():
                    test.assertEqual(names_for(model, name)["object"], ("selected", spelling, ""))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                # LLVM can produce Mach-O and COFF objects on the Linux test
                # host without foreign runtime SDKs. Check actual object
                # names, including undefined imports, independently of FBC.
                if backend == "llvm" and shutil.which("llc") and shutil.which("llvm-nm"):
                    object_file = test.working / (target + ".o")
                    triple = {"win32-x86": "i686-w64-windows-gnu", "darwin-x86_64": "x86_64-apple-darwin",
                              "linux-x86": "i686-pc-linux-gnu"}[target]
                    result = subprocess.run(["llc", "-mtriple=" + triple, "-filetype=obj", str(test.emission_path(source.stem, backend)),
                                             "-o", str(object_file)], capture_output=True, text=True, timeout=30)
                    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    result = subprocess.run(["llvm-nm", "-g", str(object_file)], capture_output=True, text=True, timeout=30)
                    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    symbols = {line.split()[-1] for line in result.stdout.splitlines() if line.split()}
                    test.assertTrue(set(expected.values()) <= symbols, symbols)

# end of emission_names.py
