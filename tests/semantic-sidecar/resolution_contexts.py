"""Project: FreeBASIC semantic sidecar tests
File: resolution_contexts.py
Purpose: Prove resolver identities come from the actual semantic graphs.
Responsibilities: Overloaded symbols, nominal types, parser expressions and real callees.
This file intentionally does NOT recover identity from spelling or memory addresses.
"""

from driver_inputs import text_rows
from recovery_steps import recovered
from semantic_resolution_contexts import FEATURE
from sidecar import Model
from resolution import NUMERIC, DEFAULTS, scopes


NOMINAL = '''namespace First
namespace Inner
type Payload
    value as long
end type
end namespace
end namespace
namespace Second
namespace Inner
type Payload
    value as long
end type
end namespace
end namespace
declare sub PickNominal overload(byref value as First.Inner.Payload)
declare sub PickNominal overload(byref value as Second.Inner.Payload)
dim actual as First.Inner.Payload
PickNominal(actual)
'''
CASTS = '''type TargetOne
    value as long
end type
type TargetTwo
    value as long
end type
type Convertible
    value as long
    declare operator cast() as TargetOne ptr
    declare operator cast() as TargetTwo ptr
end type
dim actual as Convertible
dim selected as TargetOne ptr = cptr(TargetOne ptr, actual)
'''


def context_for(model, event, module=1):
    ordinal = next(index for index, item in enumerate(model.resolution_events[module], 1) if item is event)
    return model.resolution_contexts[module, ordinal]


def check_symbols(test):
    for backend in test.backends:
        source = test.source(NUMERIC, "resolution-linked-symbols.bas")
        model = test.compile(source, backend=backend)
        test.assertEqual(model.capabilities[1][FEATURE], "available")
        scope = scopes(model, "Pick")[0]
        candidates = [model.resolution_candidates[1, identity] for identity in scope["candidates"]]
        ids = [int(context_for(model, candidate["begin"])[0]) for candidate in candidates]
        test.assertNotEqual(ids[0], ids[1])
        test.assertEqual(ids[0], ids[2])
        test.assertEqual([model.symbols[identity][2].upper() for identity in ids], ["PICK"] * 3)
        test.assertEqual(int(context_for(model, scope["end"])[0]), ids[0])
        use_key = 1, int(scope["begin"][1])
        binding = model.resolution_bound_uses[use_key]
        test.assertEqual(int(model.records["B"][binding - 1][1]), ids[0])
        test.assertEqual(model.access_roles[binding], "callee")
        test.assertEqual(model.resolution_uses[use_key][1], test.compiler_path(source))
        expressions = []
        for candidate in candidates:
            argument = candidate["arguments"][0]
            linked = context_for(model, argument)
            parameter = int(linked[0])
            test.assertEqual(model.symbols[parameter][3], "4")
            procedure = int(context_for(model, candidate["begin"])[0])
            test.assertTrue(any(int(row[2]) == parameter for row in model.parameters[procedure].values()))
            expressions.append(int(linked[3]))
        test.assertTrue(expressions[0])
        test.assertEqual(expressions, [expressions[0]] * 3)


def check_nominal(test):
    for label, source_text in (("nominal", NOMINAL), ("casts", CASTS)):
        for backend in test.backends:
            with test.subTest(backend=backend, type_context=label):
                source = test.source(source_text, "resolution-type-" + label + ".bas")
                model = test.compile(source, backend=backend)
                if label == "nominal":
                    scope = scopes(model, "PickNominal")[0]
                    screened = [model.resolution_candidates[1, identity] for identity in scope["candidates"]
                                if model.resolution_candidates[1, identity]["begin"][4] == "screen"]
                    contexts = [context_for(model, candidate["arguments"][0]) for candidate in screened]
                    test.assertNotEqual(contexts[0][1], contexts[1][1])
                    test.assertEqual(contexts[0][2], contexts[1][2])
                    # The immediate hints deliberately coincide. The canonical
                    # parent graph carries First.Inner versus Second.Inner.
                    test.assertEqual(contexts[0][4:12], contexts[1][4:12])
                    test.assertEqual(contexts[0][4:12][6:], ["PAYLOAD", "INNER"])
                    parents = [int(model.symbols[int(context[1])][11]) for context in contexts]
                    test.assertNotEqual(parents[0], parents[1])
                else:
                    scope = next(scope for scope in scopes(model) if scope["begin"][4] == "cast")
                    beginning = context_for(model, scope["begin"])
                    target = int(beginning[2])
                    test.assertEqual(model.symbols[target][2], "TARGETONE")
                    chosen = model.resolution_candidates[1, int(scope["end"][2])]
                    chosen_context = context_for(model, chosen["begin"])
                    test.assertEqual(chosen_context[1], beginning[2])
                    alternatives = [context_for(model, model.resolution_candidates[1, identity]["begin"])
                                    for identity in scope["candidates"]]
                    test.assertEqual({model.symbols[int(context[1])][2] for context in alternatives}, {"TARGETONE", "TARGETTWO"})


def check_reduced(test):
    source = test.source(NOMINAL, "resolution-reduced-types.bas")
    for mode in ("full", "expressions", "bindings"):
        model = test.compile(source, mode=mode)
        if mode == "bindings":
            test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
            test.assertFalse(model.resolution_contexts)
            test.assertFalse(model.resolution_uses)
            continue
        test.assertTrue(model.resolution_contexts)
        if mode == "expressions":
            test.assertTrue(all(fields[:3] == ["0"] * 3 for fields in model.resolution_contexts.values()))
            test.assertTrue(any(fields[4] == "1" for fields in model.resolution_contexts.values()))
            test.assertFalse(model.resolution_bound_uses)
            test.assertTrue(model.resolution_uses)
    omitted = test.compile(test.source(DEFAULTS, "resolution-linked-defaults.bas"))
    for candidate in omitted.resolution_candidates.values():
        for argument in candidate["arguments"]:
            if argument[12] == "0":
                linked = context_for(omitted, argument)
                test.assertEqual(linked[3], "0")
                test.assertEqual(linked[12], "0")
    bad = test.source('declare sub Ambiguous overload(byval first as long, byval second as double)\n'
                      'declare sub Ambiguous overload(byval first as double, byval second as long)\n'
                      'Ambiguous(1l, 1l)\n', "resolution-unbound-source.bas")
    _, rejected = recovered(test, bad)
    test.assertTrue(rejected.resolution_uses)
    test.assertFalse(rejected.resolution_bound_uses)
    test.assertTrue(all(context_for(rejected, scope["end"])[0] == "0" for scope in scopes(rejected)))


def check_macro_and_transport(test):
    source = test.source(NUMERIC.replace("Pick(1l)", "") + '#define INVOKE() Pick(1l): Pick(2l)\nINVOKE()\n',
                         "resolution-linked-macros.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        model = test.compile(source, extra=extra)
        test.assertEqual(len(model.resolution_macro_uses), 2)
        test.assertFalse(model.resolution_bound_uses)
        test.assertEqual(len(set(model.resolution_macro_uses.values())), 2)
        for key, reference in model.resolution_macro_uses.items():
            scope = model.resolution_scopes[key]
            subject = int(context_for(model, scope["end"])[0])
            test.assertIn(("symbol", subject, "reference-" + str(reference)), model.macro_origins)
            test.assertEqual(model.resolution_uses[key][7], "0")
        path = test.working / ("resolution-linked-macros-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(path)))
        binary = Model.read(path)
        test.assertEqual(binary.resolution_contexts, model.resolution_contexts)
        test.assertEqual(binary.resolution_macro_uses, model.resolution_macro_uses)
        test.assertEqual(binary.resolution_uses, model.resolution_uses)
        expression_model = test.compile(source, mode="expressions", extra=extra)
        test.assertFalse(expression_model.resolution_bound_uses)
        test.assertFalse(expression_model.resolution_macro_uses)
        test.assertTrue(expression_model.resolution_uses)
        rows = [row.copy() for row in model.rows]
        macro = next(row for row in rows if row[3:4] and row[3].startswith("native-resolution-macro-use:"))
        macro[4] = "999999"
        with test.assertRaises(ValueError):
            Model(text_rows(rows))
    plain = test.source(NUMERIC, "resolution-linked-plain.bas")
    _, path = test.invoke([plain, plain])
    repeated = Model.read(path)
    test.assertEqual({module for module, scope in repeated.resolution_uses}, {1, 2})
    test.assertEqual({module for module, scope in repeated.resolution_bound_uses}, {1, 2})


def check_corruption(test):
    model = test.compile(test.source(NUMERIC, "resolution-linked-corruption.bas"))
    for mutation in ("missing", "count", "summary", "event", "subject", "class", "expression", "absent-type", "use", "bound", "duplicate"):
        with test.subTest(resolution_context_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            context = next(row for row in rows if row[3:4] == ["native-resolution-context:1"])
            summary = next(row for row in rows if row[3:4] == ["native-resolution-context-counts"])
            if mutation in ("missing", "summary"):
                rows.remove(context if mutation == "missing" else summary)
            elif mutation == "count":
                summary[4] = "0\t0\t0"
            elif mutation == "event":
                context[3] = "native-resolution-context:999999"
            elif mutation == "duplicate":
                rows.insert(-1, context.copy())
            elif mutation == "use":
                rows.remove(next(row for row in rows if row[3:4] and row[3].startswith("native-resolution-use:")))
            elif mutation == "bound":
                next(row for row in rows if row[3:4] and row[3].startswith("native-resolution-bound-use:"))[4] = "999999"
            else:
                fields = context[4].split("\t")
                if mutation == "subject": fields[0] = "0"
                elif mutation == "class":
                    fields[0] = next(identity for identity, symbol in ((str(key), value) for key, value in model.symbols.items()) if symbol[3] == "8")
                elif mutation == "expression": fields[3] = "999999"
                elif mutation == "absent-type": fields[1] = fields[0]
                context[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of resolution_contexts.py
