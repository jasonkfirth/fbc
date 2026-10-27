"""Project: FreeBASIC semantic sidecar tests
File: namespace_visibility.py
Purpose: Prove native USING observations and their scope boundaries.
Responsibilities: Order, lifetimes, propagation, omissions, transport and corruption.
This file intentionally does NOT perform alternate namespace resolution.
"""

from driver_inputs import text_rows
from semantic_namespace_visibility import FEATURE
from sidecar import Model


def facts_for(test, backend="gcc"):
    source = test.fixture("namespace-visibility.bas")
    return source, test.compile(source, backend=backend)


def check_order_and_lifetime(test):
    source = test.fixture("namespace-visibility.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            facts = model.namespace_visibility[1]
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            requests = facts["using"]
            targets = [model.symbols[int(fields[5])][2].upper() for fields in requests.values()]
            test.assertEqual(targets, ["FIRSTSPACE", "ISOLATED", "ISOLATED", "FACADE", "FACADE", "SECONDSPACE"])
            test.assertEqual(facts["using-result"][5], ["1", "0"])
            test.assertEqual(len(facts["refresh"]), 1)
            test.assertGreater(int(facts["refresh-result"][1][0]), 0)
            scoped = [(identity, fields) for identity, fields in facts["import"].items() if fields[4] in ("2", "3")]
            test.assertEqual(len(scoped), 2)
            test.assertEqual({model.symbols[int(fields[3])][3] for _, fields in scoped}, {"3", "15"})
            for identity, fields in scoped:
                test.assertNotEqual(fields[2], fields[3])
                test.assertEqual(facts["import-state"][identity], ["removed"])
                test.assertIn(identity, facts["import-remove"])
            isolated = test.one(model, "Isolated", "namespace")
            variables = model.named("ScopedValue", "variable")
            imported_value = next(identity for identity in variables if model.symbols[identity][11] == str(isolated))
            module_value = next(identity for identity in variables if identity != imported_value)
            references = {int(row[5]): int(row[1]) for row in model.records["B"]
                          if row[2] == "reference" and int(row[1]) in variables}
            lines = source.read_text().splitlines()
            for marker, wanted in (("procedure use", imported_value), ("block use", imported_value), ("module use", module_value)):
                line = next(index for index, text in enumerate(lines, 1) if marker in text)
                test.assertEqual(references[line], wanted)
            request = requests[1]
            location = model.physical_locations["source-context", int(request[1]), "namespace-using:1"]
            test.assertEqual(source.read_bytes()[int(location[9]):int(location[10])], b"FirstSpace")
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mode in ("bindings", "expressions"):
                reduced = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(reduced.namespace_visibility)
                test.assertFalse(any(row[3].startswith(("namespace-using", "namespace-import:", "namespace-refresh",
                                                       "namespace-visibility")) for row in reduced.records["K"]))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_noops_and_source_contexts(test):
    source = test.source('namespace OuterSpace\nusing OuterSpace\nnamespace InnerSpace\n'
                         'using OuterSpace\nend namespace\nend namespace\n', "noop.bas")
    model = test.compile(source)
    facts = model.namespace_visibility[1]
    test.assertEqual(list(facts["using-result"].values()), [["0", "0"], ["0", "0"]])
    test.assertFalse(facts.get("import"))
    header = test.source("using SelectedSpace\n", "using.bi")
    source = test.source('namespace SelectedSpace\ndim shared chosen as long\nend namespace\n'
                         '#include "using.bi"\n#include "using.bi"\n'
                         '#define SELECTED SelectedSpace\nusing SELECTED\n'
                         '#if 0\nusing InactiveSpace\n#endif\nprint chosen\n', "contexts.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            facts = model.namespace_visibility[1]
            test.assertEqual(len(facts["using"]), 3)
            test.assertNotEqual(facts["using"][1][1], facts["using"][2][1])
            test.assertEqual(facts["using"][3][8], "expanded")
            test.assertNotIn(("source-context", int(facts["using"][3][1]), "namespace-using:3"), model.physical_locations)
            test.assertEqual(facts["using-result"][2], ["1", "0"])
            test.assertFalse(model.named("InactiveSpace", "namespace"))
            test.assertIn(test.compiler_path(header), [row[1] for row in model.records["D"]])
    invalid = test.source('namespace Hidden\ndim shared hidden_value as long\nend namespace\n'
                          'scope\nusing Hidden\nprint hidden_value\nend scope\nprint hidden_value\n', "scope-error.bas")
    baseline, _ = test.invoke([invalid], mode="off", success=False)
    observed, artifact = test.invoke([invalid], mode="full", success=False)
    test.assertEqual((observed.returncode, observed.stdout, observed.stderr),
                     (baseline.returncode, baseline.stdout, baseline.stderr))
    with test.assertRaises(ValueError):
        Model.read(artifact)


def check_transport_and_modules(test):
    source, text = facts_for(test)
    artifact = test.working / "namespace.bin"
    test.invoke([source], mode="off", extra=("-semantic-model", str(artifact), "-o", str(source.with_suffix(".o"))))
    test.assertEqual(Model.read(artifact).rows, text.rows)
    secondary = test.source("namespace OtherSpace\nend namespace\nusing OtherSpace\n", "secondary.bas")
    _, artifact = test.invoke([source, secondary])
    multiple = Model.read(artifact)
    test.assertEqual(set(multiple.namespace_visibility), {1, 2})
    test.assertEqual(set(multiple.namespace_visibility[2]["using"]), {1})
    test.assertNotEqual(multiple.namespace_visibility[1]["owner"], multiple.namespace_visibility[2]["owner"])
    empty = test.compile(test.source("print 1\n", "empty.bas"))
    test.assertEqual(empty.namespace_visibility[1]["visibility-counts"][1], ["0"] * 4)


def check_rejection(test):
    _, model = facts_for(test)
    imported = next(identity for identity, fields in model.namespace_visibility[1]["import"].items() if fields[4] == "2")
    mutations = ("count", "huge-count", "missing-request", "missing-outcome", "missing-state", "missing-link",
                 "request-owner", "target", "destination", "lexical-owner", "source", "bindings", "request-link",
                 "refresh-link", "missing-refresh-result", "invent-created", "state", "origin", "unavailable")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def prop(key):
                return next(row for row in rows if row[0] == "K" and row[3] == key)
            def change(key, index, value):
                row = prop(key)
                fields = row[4].split("\t")
                fields[index] = value
                row[4] = "\t".join(fields)
            if mutation in ("count", "huge-count"):
                change("namespace-visibility-counts:1", 0, "0" if mutation == "count" else "1000001")
            elif mutation.startswith("missing-"):
                if mutation == "missing-link":
                    native = model.namespace_visibility[1]["import"][imported][0]
                    rows = [row for row in rows if not (row[0] == "H" and row[1:3] == ["symbol", native] and row[5] == "imports")]
                else:
                    rows.remove(prop({"missing-request": "namespace-using:1", "missing-outcome": "namespace-using-result:1",
                                      "missing-state": f"namespace-import-state:{imported}",
                                      "missing-refresh-result": "namespace-refresh-result:1"}[mutation]))
            elif mutation == "request-owner":
                prop("namespace-using:1")[2] = str(test.one(model, "Facade", "namespace"))
            elif mutation in ("target", "destination", "lexical-owner", "request-link", "refresh-link", "origin"):
                index = {"target": 1, "destination": 2, "lexical-owner": 3, "request-link": 4, "refresh-link": 5, "origin": 9}[mutation]
                value = "preexisting" if mutation == "origin" else "999999"
                if mutation == "lexical-owner":
                    value = str(model.namespace_visibility[1]["owner"])
                change(f"namespace-import:{imported}", index, value)
            elif mutation == "source":
                change("namespace-using:1", 1, "999999")
            elif mutation == "bindings":
                change("namespace-using:1", 7, "999999")
            elif mutation == "invent-created":
                change("namespace-using-result:5", 1, "1")
            elif mutation == "state":
                prop(f"namespace-import-state:{imported}")[4] = "retained"
            else:
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            Model(text_rows(rows))


def check_binding_contexts(test):
    source = test.source('namespace ContextSpace\n'
                         'type Holder\ntype NestedPart\nnested_value as long\nend type\n'
                         'union\nleft_value as long\nright_value as long\nend union\n'
                         'declare function GetValue() as long\nend type\n'
                         'enum ScopedValues explicit\nOne = 1\nend enum\n'
                         'enum\nAnonymousValue = 2\nend enum\nend namespace\n'
                         'function ContextSpace.Holder.GetValue() as long\nscope\n'
                         'dim local_value as long\nlocal_value = this.left_value\nreturn local_value\n'
                         'end scope\nend function\n'
                         'namespace ContextSpace\ndim item as Holder\nend namespace\n'
                         'print ContextSpace.item.GetValue(), ContextSpace.ScopedValues.One, ContextSpace.AnonymousValue\n',
                         "binding-contexts.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            contexts = model.binding_scope_contexts[1]
            test.assertEqual(set(contexts), set(range(1, len(model.records["B"]) + 1)))
            owners = {model.symbols[int(fields[3])][3] for fields in contexts.values()}
            test.assertTrue({"3", "8", "9", "10", "15"} <= owners, owners)
            test.assertTrue(any(fields[3] != fields[4] for fields in contexts.values()))
            anonymous = {int(row[2]) for row in model.relations("anonymous-parent") if row[1] == row[3] == "symbol"}
            test.assertTrue(anonymous)
            # Anonymous aggregates change layout ownership, while their
            # fields remain in the enclosing native lexical/lookup table.
            # Preserve the written UNION occurrence without inventing a
            # separate namespace for the generated aggregate symbol.
            test.assertFalse(any(int(fields[3]) in anonymous for fields in contexts.values()))
            unions = {str(identity) for identity, block in model.constructs.items() if block[5] == "union"}
            test.assertTrue(unions)
            test.assertTrue(any(fields[2] in unions for fields in contexts.values()))
            nested = test.one(model, "NestedPart", "type")
            holder = test.one(model, "Holder", "type")
            nested_value = test.one(model, "nested_value", "field")
            nested_binding = next(identity for identity, binding in enumerate(model.records["B"], 1)
                                  if binding[1:3] == [str(nested_value), "declaration"])
            test.assertEqual(model.symbols[nested_value][11], str(nested))
            test.assertEqual(contexts[nested_binding][3:6], (str(holder),) * 3)
            test.assertEqual(model.constructs[int(contexts[nested_binding][2])][5], "type")
            # Unscoped enums still enter a native enum table, then import it
            # into the enclosing namespace. That implicit relationship is
            # not a written USING request.
            implicit_enums = [fields for fields in model.namespace_visibility[1]["import"].values()
                              if model.symbols[int(fields[1])][3] == "9"]
            test.assertEqual(len(implicit_enums), 1)
            test.assertEqual(implicit_enums[0][4:6], ["0", "0"])
            space = test.one(model, "ContextSpace", "namespace")
            # Reopened source blocks have distinct IDs even though native
            # lexical/lookup symbols are the same namespace identity.
            blocks = [int(row[3].split(":")[1]) for row in model.records["K"]
                      if row[3].startswith("namespace-declaration:") and row[4].split("\t")[1] == str(space)]
            test.assertEqual(len(blocks), 2)
            test.assertNotEqual(blocks[0], blocks[1])
            test.assertTrue(all(any(fields[2] == str(block) for fields in contexts.values()) for block in blocks))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mutation in ("missing", "lexical", "lookup", "hash", "statement", "source", "construct", "count", "late"):
                with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                    rows = [row.copy() for row in model.rows]
                    record = next(row for row in rows if row[0] == "K" and row[3].startswith("namespace-binding:"))
                    if mutation == "missing":
                        rows.remove(record)
                    elif mutation == "count":
                        next(row for row in rows if row[0] == "K" and row[3] == "namespace-binding-count:1")[4] = "0"
                    elif mutation == "late":
                        rows.remove(record)
                        rows.insert(-1, record)
                    else:
                        fields = record[4].split("\t")
                        slot = {"statement": 0, "source": 1, "construct": 2, "lexical": 3, "lookup": 4, "hash": 5}[mutation]
                        fields[slot] = "999999"
                        record[4] = "\t".join(fields)
                    Model(text_rows(rows))

# end of namespace_visibility.py
