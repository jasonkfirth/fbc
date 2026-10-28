"""Project: FreeBASIC semantic sidecar tests
File: object_model.py
Purpose: Prove actual object-table contents, inheritance and RTTI phase meaning.
Responsibilities: Targets, abstract optimization, exclusions and damaged facts.
This file intentionally does NOT infer runtime targets from a static receiver.
"""

import subprocess

from driver_inputs import text_rows
from semantic_object_model import FEATURE
from sidecar import Model


def build_for(test, model, name):
    owner = test.one(model, name, "type")
    return next((identity, row) for identity, row in model.object_models[1]["build"].items() if row[0] == str(owner))


def method(test, model, owner_name, name):
    owner = test.one(model, owner_name, "type")
    return next(identity for identity in model.named(name, "procedure") if model.types[identity][8] == str(owner))


def check_tables(test):
    source = test.fixture("object-model.bas")
    for backend in test.backends:
        for target in ("linux-x86", "linux-x86_64", "win32-x86", "win32-x86_64"):
            for optimized in (False, True):
                with test.subTest(backend=backend, target=target, optimized=optimized):
                    extra = ("-target", target, *(("-z", "optabstract") if optimized else ()))
                    test.invoke([source], mode="off", backend=backend, extra=extra)
                    emission = test.emission_path(source.stem, backend).read_bytes()
                    model = test.compile(source, backend=backend, extra=extra)
                    facts = model.object_models[1]
                    test.assertEqual(model.capabilities[1][FEATURE], "available")
                    width = 8 if target.endswith("x86_64") else 4
                    for identity, build in facts["build"].items():
                        selected = facts["slots"][identity]
                        test.assertEqual(selected[0][:2], ("null", "0"))
                        test.assertEqual(selected[1][:2], ("address", build[2]))
                        test.assertEqual([int(row[2]) for row in selected.values()], list(range(0, len(selected) * width, width)))
                        test.assertTrue(all(row[3] == str(width) for row in selected.values()))
                    empty, header = build_for(test, model, "EmptyObjectTable")
                    test.assertEqual(header[5], "2")
                    test.assertFalse(facts["members"][empty])
                    pure, header = build_for(test, model, "PureObjectBase")
                    test.assertEqual([facts["slots"][pure][index][0] for index in (2, 3)], ["null", "null"])
                    test.assertEqual(header[8], "1" if optimized else "0")
                    pure_owner = header[0]
                    history = [row for row in facts["rtti"].values() if row[0] == pure_owner]
                    test.assertEqual([row[2] for row in history], ["deferred", "activated"] if optimized else ["initialized"])
                    if optimized:
                        test.assertEqual(history[0][4:6], ("0", "0"))
                        test.assertEqual(history[0][6:], ("unavailable", "0", "", "", ""))
                    partial_read = method(test, model, "PartialObject", "ReadValue")
                    concrete_update = method(test, model, "ConcreteObject", "Update")
                    partial, header = build_for(test, model, "PartialObject")
                    test.assertEqual(header[6:8], ("0", "0") if optimized else ("4", "2"))
                    test.assertEqual(facts["slots"][partial][2][1], str(partial_read))
                    test.assertEqual(facts["slots"][partial][3][0], "null")
                    concrete, _ = build_for(test, model, "ConcreteObject")
                    test.assertEqual([facts["slots"][concrete][index][1] for index in (2, 3)], [str(partial_read), str(concrete_update)])
                    leaf_read = method(test, model, "LeafObject", "ReadValue")
                    leaf_extra = method(test, model, "LeafObject", "ExtraValue")
                    leaf, leaf_header = build_for(test, model, "LeafObject")
                    inherited, header = build_for(test, model, "InheritedObject")
                    test.assertEqual([facts["slots"][inherited][index][1] for index in (2, 3, 4)],
                                     [str(leaf_read), str(concrete_update), str(leaf_extra)])
                    test.assertTrue(all(row["copied_from"] == int(leaf_header[1]) for row in facts["origins"][inherited].values()))
                    again, _ = build_for(test, model, "AbstractAgainObject")
                    again_read = method(test, model, "AbstractAgainObject", "ReadValue")
                    test.assertEqual(facts["slots"][again][2][1], str(partial_read))
                    test.assertEqual(facts["origins"][again][2]["declaration"], again_read)
                    test.assertEqual(facts["origins"][again][2]["selection"], "inherited")
                    new_abstract, _ = build_for(test, model, "NewAbstractObject")
                    test.assertEqual(facts["slots"][new_abstract][5][:2], ("null", "0"))
                    # Destructor slots retain the selected native procedure kind,
                    # including any deleting wrapper the compiler actually adds.
                    lifetime, _ = build_for(test, model, "VirtualLifetimeChild")
                    selected_dtors = [model.signatures[int(row[1])][2] for index, row in facts["slots"][lifetime].items() if index >= 2 and row[1] != "0"]
                    test.assertIn("destructor", selected_dtors)
                    lifetime_owner = test.one(model, "VirtualLifetimeChild", "type")
                    test.assertFalse(any(row[2] == str(lifetime_owner) for row in model.relations("deleting-destructor")))
                    for name, kind in (("PlainObjectData", "type"), ("PlainObjectUnion", "union")):
                        owner = test.one(model, name, kind)
                        observed = [row for identity, row in facts["snapshot"].values() if identity == owner]
                        test.assertTrue(observed)
                        test.assertTrue(all(row[0] == "plain" and row[2:4] == ("0", "0") for row in observed))
                        test.assertFalse(any(row[0] == str(owner) for row in facts["build"].values()))
                    test.assertTrue(any(row[0] == "runtime" and row[2] == "0" for owner, row in facts["snapshot"].values()))
                    test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_omissions(test):
    source = test.fixture("object-model.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            model = test.compile(source, backend=backend, mode=mode)
            test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
            test.assertFalse(model.object_models)
    for backend, target in (("gas", "linux-x86"), ("gas64", "linux-x86_64")):
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend, extra=("-target", target))
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertTrue(model.object_models[1]["build"])


def check_transport(test):
    source = test.fixture("object-model.bas")
    for abi in ("native", "stable-v1"):
        model = test.compile(source, extra=("-semantic-type-abi", abi))
        artifact = test.working / ("object-" + abi + ".bin")
        test.invoke([source], mode="off", extra=("-semantic-type-abi", abi, "-semantic-model", str(artifact),
                                                "-o", str(source.with_suffix(".o"))))
        test.assertEqual(Model.read(artifact).rows, model.rows)
    first = test.source("type FirstObject extends object\nend type\n", "first-object.bas")
    second = test.source("type SecondObject extends object\nend type\n", "second-object.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    test.assertEqual(set(model.object_models), {1, 2})
    test.assertEqual(next(iter(model.object_models[2]["build"])), 1)
    test.assertEqual(next(iter(model.object_models[2]["snapshot"])), 1)


def check_rejection(test):
    model = test.compile(test.fixture("object-model.bas"), extra=("-z", "optabstract"))
    mutations = ("state", "has-rtti", "snapshot-base", "snapshot-abstract", "plain-table", "build-owner", "build-base", "copy-count",
                 "slot-kind", "slot-target", "slot-offset", "slot-size", "member-slot", "member-abstract", "member-override",
                 "rtti-state", "rtti-base", "rtti-name", "rtti-offset", "deferred-name", "end-count", "counts", "missing-slot",
                 "missing-member", "missing-snapshot", "missing-rtti", "unavailable")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def packet(prefix, predicate=lambda fields: True):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(prefix)
                            and predicate(row[4].split("\t")))
            if mutation == "unavailable":
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            elif mutation.startswith("missing-"):
                family = mutation.removeprefix("missing-")
                prefix = {"slot": "virtual-table-slot:", "member": "virtual-table-member:",
                          "snapshot": "object-model-snapshot:", "rtti": "rtti-initialization:"}[family]
                rows.remove(packet(prefix))
            else:
                if mutation in ("state", "has-rtti", "snapshot-base", "snapshot-abstract", "plain-table"):
                    row = packet("object-model-snapshot:", lambda fields: fields[0] == "plain" if mutation == "plain-table" else fields[0] == "owned")
                    slot = {"state": 0, "snapshot-base": 1, "snapshot-abstract": 5, "has-rtti": 8, "plain-table": 2}[mutation]
                    replacement = {"state": "pretend", "has-rtti": "0", "plain-table": str(next(iter(model.object_models[1]["build"].values()))[1])}.get(mutation, "999999")
                elif mutation.startswith("build-") or mutation == "copy-count":
                    row = packet("virtual-table-build:")
                    slot = {"build-owner": 0, "build-base": 3, "copy-count": 7}[mutation]
                    replacement = "999999"
                elif mutation.startswith("slot-"):
                    row = packet("virtual-table-slot:")
                    slot = {"slot-kind": 0, "slot-target": 1, "slot-offset": 2, "slot-size": 3}[mutation]
                    replacement = "pretend" if mutation == "slot-kind" else "999999"
                elif mutation.startswith("member-"):
                    row = packet("virtual-table-member:")
                    slot = {"member-slot": 0, "member-abstract": 2, "member-override": 3}[mutation]
                    replacement = "0" if mutation == "member-abstract" else "999999"
                elif mutation.startswith("rtti-") or mutation == "deferred-name":
                    row = packet("rtti-initialization:", lambda fields: fields[2] == "deferred" if mutation == "deferred-name" else fields[2] != "deferred")
                    slot = {"rtti-state": 2, "rtti-base": 4, "rtti-name": 5, "rtti-offset": 8, "deferred-name": 5}[mutation]
                    replacement = "pretend" if mutation == "rtti-state" else "999999"
                else:
                    row = packet("object-model-counts:" if mutation == "counts" else "virtual-table-end:")
                    slot, replacement = 0, "999999"
                fields = row[4].split("\t")
                fields[slot] = replacement
                row[4] = "\t".join(fields)
            Model(text_rows(rows))


def check_execution(test):
    source = test.fixture("object-model.bas")
    for optimized in (False, True):
        for mode in ("off", "full", "bindings", "expressions"):
            with test.subTest(optimized=optimized, mode=mode):
                executable = test.working / ("objects-" + str(int(optimized)) + "-" + mode)
                extra = (*(("-z", "optabstract") if optimized else ()), "-x", str(executable))
                test.invoke([source], mode=mode, emit=False, extra=extra)
                result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
                test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                test.assertEqual(result.stdout.strip(), "OBJECT_MODEL_PASS")


def check_guards(test):
    executable = test.working / "object-model-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("object-model-guards.bas")), str(test.root / "src/compiler/tooling/semantic-object-model.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "OBJECT_MODEL_GUARDS_PASS")


def check_namespaces_and_unused(test):
    source = test.source('namespace ObjectLeft\n'
                         'type Repeated extends object\nend type\n'
                         'end namespace\nnamespace ObjectRight\n'
                         'type Repeated extends ObjectLeft.Repeated\nend type\n'
                         'end namespace\n'
                         'type UnusedPureObject extends object\n'
                         'declare abstract sub Run()\nend type\n', "object-namespaces.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend, extra=("-z", "optabstract"))
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=("-z", "optabstract"))
            facts = model.object_models[1]
            repeated = model.named("Repeated", "type")
            test.assertEqual(len(repeated), 2)
            rtti = {int(row[0]): row for row in facts["rtti"].values()}
            names = [model.constants["symbol", int(rtti[owner][5])][1] for owner in repeated]
            test.assertEqual(len(set(names)), 2)
            test.assertNotEqual(model.types[repeated[0]][8], model.types[repeated[1]][8])
            derived = next(owner for owner in repeated if rtti[owner][3] == str(next(value for value in repeated if value != owner)))
            base = int(rtti[derived][3])
            test.assertEqual(rtti[derived][4], rtti[base][1])
            unused = test.one(model, "UnusedPureObject", "type")
            history = [row for row in facts["rtti"].values() if row[0] == str(unused)]
            test.assertEqual([row[2] for row in history], ["deferred"])
            snapshot = [row for owner, row in facts["snapshot"].values() if owner == unused][-1]
            test.assertEqual(snapshot[6:8], ("1", "1"))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)


def check_recounted_rejection(test):
    model = test.compile(test.fixture("object-model.bas"), extra=("-z", "optabstract"))
    for mutation in ("snapshot", "abstract-member", "whole-build", "activated-rtti"):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            closure = next(row for row in rows if row[0] == "K" and row[3] == "object-model-counts:1")
            counts = closure[4].split("\t")
            if mutation == "snapshot":
                row = next(row for row in reversed(rows) if row[0] == "K" and row[3].startswith("object-model-snapshot:"))
                rows.remove(row)
                counts[0] = str(int(counts[0]) - 1)
            elif mutation == "abstract-member":
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("virtual-table-member:")
                           and row[4].split("\t")[2] == "1")
                build = row[3].split(":")[1]
                rows.remove(row)
                counts[3] = str(int(counts[3]) - 1)
                prefix = "virtual-table-member:" + build + ":"
                for ordinal, value in enumerate((value for value in rows if value[0] == "K" and value[3].startswith(prefix)), 1):
                    value[3] = prefix + str(ordinal)
                end = next(value for value in rows if value[0] == "K" and value[3] == "virtual-table-end:" + build)
                fields = end[4].split("\t"); fields[1] = str(int(fields[1]) - 1); end[4] = "\t".join(fields)
            elif mutation == "whole-build":
                build = str(max(model.object_models[1]["build"]))
                prefixes = ("virtual-table-build:" + build, "virtual-table-end:" + build)
                components = ("virtual-table-slot:" + build + ":", "virtual-table-member:" + build + ":")
                removed = [row for row in rows if row[0] == "K" and (row[3] in prefixes or row[3].startswith(components))]
                counts[1] = str(int(counts[1]) - 1)
                counts[2] = str(int(counts[2]) - sum(row[3].startswith(components[0]) for row in removed))
                counts[3] = str(int(counts[3]) - sum(row[3].startswith(components[1]) for row in removed))
                rows = [row for row in rows if row not in removed]
                closure = next(row for row in rows if row[0] == "K" and row[3] == "object-model-counts:1")
            else:
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("rtti-initialization:")
                           and row[4].split("\t")[2] == "activated")
                rows.remove(row)
                counts[4] = str(int(counts[4]) - 1)
                for ordinal, value in enumerate((value for value in rows if value[0] == "K" and value[3].startswith("rtti-initialization:")), 1):
                    value[3] = "rtti-initialization:" + str(ordinal)
            closure[4] = "\t".join(counts)
            Model(text_rows(rows))


def check_cpp_and_architectures(test):
    source = test.source('extern "c++"\n'
                         'type CppObjectBase extends object\n'
                         'declare virtual destructor()\nend type\n'
                         'type CppObjectChild extends CppObjectBase\n'
                         'payload as string\nend type\nend extern\n'
                         'destructor CppObjectBase()\nend destructor\n'
                         'dim instance as CppObjectChild\n', "cpp-object-model.bas")
    choices = [(backend, "linux-x86_64") for backend in test.backends]
    choices += [("gcc", target) for target in ("amiga-m68k", "riscos-arm", "wince-arm", "linux-aarch64")]
    for backend, target in choices:
        with test.subTest(backend=backend, target=target):
            extra = ("-target", target)
            test.invoke([source], mode="off", backend=backend, extra=extra)
            emission = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            owner = test.one(model, "CppObjectChild", "type")
            deleting = [row for row in model.relations("deleting-destructor") if row[2] == str(owner)]
            test.assertTrue(deleting)
            selected = {int(row[4]) for row in deleting}
            test.assertEqual(len(selected), 1)
            proc = selected.pop()
            # F keeps the destructor family; the self-described procedure
            # attribute identifies the generated deleting wrapper precisely.
            test.assertEqual(model.signatures[proc][2], "destructor")
            deleting_bit = next(int(info[2]) for info in model.flag_definitions[1].values()
                                if info[:2] == ["procedure-attributes", "deleting-destructor"])
            test.assertTrue(int(model.types[proc][11]) & deleting_bit)
            # The base has only its complete destructor at slot two. This
            # generated C++ deleting wrapper receives the next native slot.
            test.assertEqual(model.signatures[proc][12], "3")
            identity, _ = build_for(test, model, "CppObjectChild")
            test.assertEqual(model.object_models[1]["slots"][identity][3][1], str(proc))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)

# end of object_model.py
