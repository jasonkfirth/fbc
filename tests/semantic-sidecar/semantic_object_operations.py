"""Project: FreeBASIC semantic sidecar
File: semantic_object_operations.py
Purpose: Validate selected native construction, copying and destruction work.
Responsibilities: Native nesting, choices, selected calls, roots and module closure.
This file intentionally does NOT interpret builder order as runtime execution.
"""

from collections import defaultdict

FEATURE = "native-object-operations"
BASE_FEATURE = "native-base-subobject-fields"
LIMIT = 1000000
KINDS = frozenset(("constructor-initialization", "base-initialization", "field-initialization", "union-initialization",
                   "vptr-initialization", "complete-destruction", "field-destruction", "base-destruction",
                   "deleting-destruction", "result-initialization", "default-constructor-call", "destructor-call",
                   "implicit-construction", "reused-construction", "constructor-chain", "base-initializer-source",
                   "generated-member-body", "generated-member-declaration", "generated-assignment-field",
                   "generated-union-copy", "field-exclusion"))
OUTCOMES = frozenset(("built", "empty", "shallow", "error", "reused", "skipped", "complete"))
PREFIXES = ("native-object-operation:", "native-object-end:", "native-object-choice:",
            "native-object-selection:", "native-object-nodes:", "native-object-counts:")
CHOICES = {"field-init": {"fake-dynamic-skip", "no-initialize", "default-constructor", "default-constructor-array", "zero-bitfield", "zero-storage"},
           "base-init": {"no-base", "explicit", "default-constructor", "missing-required-constructor", "plain-storage"},
           "dispatch": {"direct", "virtual"}, "access": {"accessible", "inaccessible"},
           "copy-route": {"dynamic-array", "scalar", "fixed-array"}, "copy-source": {"const", "mutable"}}


def validate_object_operations(model, number, subject_modules, node_kinds):
    modules = {}
    module = 0
    static_targets = {int(row[2]): int(row[4]) for row in model.relations("static-target") if row[1] == "node"}
    bodies = {int(row[2]) for row in model.records["PH"]}
    anonymous_parents = {int(row[2]): int(row[4]) for row in model.relations("anonymous-parent")}
    ownership_cache = {}
    ownership_work = 8000000
    base_fields = {}
    for relation in model.relations("base-field"):
        aggregate, field = int(relation[2]), int(relation[4])
        target = model.symbols.get(field)
        owner = model.symbols.get(aggregate)
        source_module = subject_modules.get(("symbol", aggregate))
        if (relation[1] != "symbol" or relation[3] != "symbol" or relation[6] != "0"
                or owner is None or owner[3] not in ("10", "11") or target is None or target[3] != "12"
                or target[11] != str(aggregate) or subject_modules.get(("symbol", field)) != source_module
                or model.capabilities.get(source_module, {}).get(BASE_FEATURE) != "available"):
            raise ValueError("Base subobject field is foreign or lacks its native aggregate")
        if aggregate in base_fields and base_fields[aggregate] != field:
            raise ValueError("Aggregate changes its native base subobject field")
        base_fields[aggregate] = field
    model.base_subobject_fields = base_fields
    for source_module, features in model.capabilities.items():
        if features.get(BASE_FEATURE) != "available":
            continue
        expected = {identity for identity, layout in model.layouts.items()
                    if int(layout[3]) and subject_modules.get(("symbol", identity)) == source_module}
        actual = {identity for identity in base_fields if subject_modules.get(("symbol", identity)) == source_module}
        if expected != actual:
            raise ValueError("Base subobject field coverage is incomplete")
        for aggregate in actual:
            if model.symbols[base_fields[aggregate]][5] != model.layouts[aggregate][3]:
                raise ValueError("Base storage field has another declared base type")

    def owns_field(field, aggregate):
        # The native visitor flattens anonymous aggregates into its member
        # list while S retains each field's real lexical owner. Follow only
        # the compiler's explicit anonymous-parent edges across that boundary.
        nonlocal ownership_work
        parent = int(model.symbols[field][11])
        key = parent, aggregate
        if key in ownership_cache:
            return ownership_cache[key]
        visited = set()
        while parent != aggregate and parent in anonymous_parents:
            ownership_work -= 1
            if ownership_work < 0 or parent in visited:
                raise ValueError("Anonymous field ownership exceeds its bound or contains a cycle")
            visited.add(parent)
            parent = anonymous_parents[parent]
        ownership_cache[key] = parent == aggregate
        return ownership_cache[key]

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Object operation scalar is not canonical or bounded")
        return result

    def symbol(value, classes=None, nullable=False):
        identity = integer(value)
        if identity == 0 and nullable:
            return 0
        row = model.symbols.get(identity)
        if (row is None or subject_modules.get(("symbol", identity)) != module
                or classes is not None and row[3] not in classes):
            raise ValueError("Object operation symbol has another class or module")
        return identity

    def state():
        if module not in modules:
            modules[module] = {"operations": {}, "ends": {}, "choices": defaultdict(list), "selections": {},
                               "projections": defaultdict(set), "node_counts": {}, "stack": [], "closure": None}
        return modules[module]

    def scope(fields):
        statement, source, configuration = (integer(value) for value in fields)
        if statement and (statement not in model.statements or model.statements[statement][7] != str(module)):
            raise ValueError("Object operation statement belongs elsewhere")
        if source and (source not in model.source_contexts or model.source_contexts[source][4] != str(module)):
            raise ValueError("Object operation source belongs elsewhere")
        if configuration and model.configurations.get(configuration) != module:
            raise ValueError("Object operation configuration belongs elsewhere")

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Object operation has unavailable coverage")
        facts = state()
        if facts["closure"] is not None:
            raise ValueError("Object operation follows its module closure")
        parts = row[3].split(":")
        family = parts[0]
        identity = integer(parts[1], 1, LIMIT)
        if row[1] == "node":
            if family != "native-object-operation" or len(parts) != 2 or row[4] != "1":
                raise ValueError("Malformed object operation root link")
            node = integer(row[2], 1)
            if node not in model.nodes or subject_modules.get(("node", node)) != module or identity not in facts["ends"]:
                raise ValueError("Object operation root is foreign, dangling or unfinished")
            if node in facts["projections"][identity]:
                raise ValueError("Repeated object operation root link")
            facts["projections"][identity].add(node)
            continue
        owner = symbol(row[2], {"8"})
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Object operation lacks its module namespace owner")
        fields = row[4].split("\t")
        if family == "native-object-operation":
            if len(parts) != 2 or identity != len(facts["operations"]) + 1 or len(fields) != 11 or fields[1] not in KINDS:
                raise ValueError("Malformed or unordered object operation")
            parent = integer(fields[0], 0, LIMIT)
            if parent != (facts["stack"][-1] if facts["stack"] else 0):
                raise ValueError("Object operation parent differs from native construction nesting")
            procedure = symbol(fields[2], {"3"}, True)
            aggregate = symbol(fields[3], {"10", "11"})
            target = symbol(fields[4], nullable=True)
            selected = symbol(fields[5], {"3"}, True)
            options = integer(fields[6], -1)
            integer(fields[7], -1)
            scope(fields[8:])
            kind = fields[1]
            field_kinds = {"field-initialization", "union-initialization", "field-destruction", "generated-assignment-field", "generated-union-copy"}
            if kind in field_kinds and (not target or model.symbols[target][3] != "12" or not owns_field(target, aggregate)):
                raise ValueError("Object operation does not own its selected field")
            if kind in ("base-initialization", "base-destruction", "base-initializer-source") and target:
                if model.symbols[target][3] != "12" or not owns_field(target, aggregate):
                    raise ValueError("Object operation base has another field owner")
            if (kind in ("base-initialization", "base-destruction", "base-initializer-source")
                    and model.capabilities.get(module, {}).get(BASE_FEATURE) == "available"
                    and target != base_fields.get(aggregate, 0)):
                raise ValueError("Object operation does not use its actual base storage field")
            if kind in ("generated-member-body", "generated-member-declaration"):
                if not target or target != selected or model.symbols[target][11] != str(aggregate) or model.types.get(target, [""] * 19)[18] != "compiler":
                    raise ValueError("Generated member operation invents a source member or another aggregate")
            if kind == "result-initialization" and (not target or model.symbols[target][3] != "1"):
                raise ValueError("Result construction lacks its actual result variable")
            if kind == "field-initialization" and options not in (0, 1, 2):
                raise ValueError("Unknown native field initializer route")
            if kind == "field-exclusion":
                if not target or options not in (0, 1, 2) or options == 0 and model.symbols[target][3] == "12":
                    raise ValueError("Unknown field exclusion or an instance mislabeled as another member")
            if kind == "implicit-construction" and not -1 <= options <= 4:
                raise ValueError("Unknown implicit construction argument mode")
            if kind not in ("implicit-construction", "generated-member-declaration") and int(fields[7]) != 0:
                raise ValueError("Object operation invents a native selection diagnostic")
            if kind == "implicit-construction" and int(fields[7]) < 0:
                raise ValueError("Constructor selection has a negative native error code")
            if kind in ("destructor-call", "generated-member-body") and options not in (0, 1):
                raise ValueError("Unknown special member construction flags")
            if kind not in ("implicit-construction", "field-initialization", "field-exclusion", "generated-member-declaration", "generated-member-body", "destructor-call") and options:
                raise ValueError("Object operation has another route's native options")
            facts["operations"][identity] = fields
            facts["stack"].append(identity)
        elif family == "native-object-end":
            if len(parts) != 2 or len(fields) != 4 or not facts["stack"] or facts["stack"][-1] != identity:
                raise ValueError("Object operation has an unmatched native return")
            has_root = integer(fields[0], 0, 1)
            raw_class = integer(fields[1])
            if not has_root and raw_class != 0 or has_root and raw_class >= len(node_kinds) or fields[2] not in OUTCOMES:
                raise ValueError("Object operation root class or outcome is invalid")
            if fields[2] in ("built", "shallow", "reused") and not has_root or fields[2] in ("empty", "error", "skipped", "complete") and has_root:
                raise ValueError("Object operation outcome contradicts its native result")
            if integer(fields[3], 0, LIMIT) != len(facts["choices"][identity]):
                raise ValueError("Object operation loses native choices")
            facts["ends"][identity] = fields
            facts["stack"].pop()
        elif family == "native-object-choice":
            if len(parts) != 3 or len(fields) != 2 or not facts["stack"] or facts["stack"][-1] != identity:
                raise ValueError("Object operation choice lacks its active frame")
            if integer(parts[2], 1, LIMIT) != len(facts["choices"][identity]) + 1:
                raise ValueError("Object operation choice order is not dense")
            name, value = fields
            choice_owners = {"field-init": {"field-initialization", "base-initialization"},
                             "base-init": {"base-initialization"}, "dispatch": {"destructor-call"},
                             "access": {"implicit-construction"},
                             "copy-route": {"generated-assignment-field"}, "copy-source": {"generated-assignment-field"},
                             "union-member": {"union-initialization", "generated-union-copy"},
                             "union-bytes": {"union-initialization", "generated-union-copy"}, "vptr-table": {"vptr-initialization"}}
            if facts["operations"][identity][1] not in choice_owners.get(name, set()):
                raise ValueError("Object choice belongs to another operation kind")
            if name in CHOICES:
                if value not in CHOICES[name]:
                    raise ValueError("Unknown native object choice")
            elif name == "union-member":
                target = symbol(value, {"12"})
                aggregate = int(facts["operations"][identity][3])
                if not owns_field(target, aggregate):
                    raise ValueError("Union operation includes another aggregate's field")
            elif name == "union-bytes":
                integer(value)
            elif name == "vptr-table":
                symbol(value, {"1"})
            else:
                raise ValueError("Unknown object operation choice category")
            facts["choices"][identity].append(fields)
        elif family == "native-object-selection":
            if len(parts) != 2 or len(fields) != 2 or not facts["stack"] or facts["stack"][-1] != identity or identity in facts["selections"]:
                raise ValueError("Object operation repeats or loses its selected call")
            selected = symbol(fields[0], {"3"}, True)
            raw_class = integer(fields[1])
            if (raw_class >= len(node_kinds) or not selected or node_kinds[raw_class] != "call"
                    or facts["operations"][identity][1] not in ("constructor-chain", "base-initializer-source")):
                raise ValueError("Object operation selection is not its actual constructor call")
            facts["selections"][identity] = fields
        elif family == "native-object-nodes":
            if len(parts) != 2 or identity not in facts["ends"] or identity in facts["node_counts"] or facts["stack"]:
                raise ValueError("Object operation root closure is missing, repeated or premature")
            facts["node_counts"][identity] = integer(row[4], 0, LIMIT)
        elif family == "native-object-counts":
            if len(parts) != 2 or identity != module or len(fields) != 5 or facts["stack"]:
                raise ValueError("Malformed object operation module closure")
            facts["closure"] = [integer(value, 0, LIMIT) for value in fields]
        else:
            raise ValueError("Unknown native object operation record")

    model.native_object_operations = modules
    for module in range(1, len(model.records["M"]) + 1):
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            continue
        facts = modules.get(module)
        if facts is None:
            raise ValueError("Missing native object operation module closure")
        events, ends = facts["operations"], facts["ends"]
        totals = [len(events), len(ends), sum(map(len, facts["choices"].values())), len(facts["selections"]),
                  sum(map(len, facts["projections"].values()))]
        if facts["closure"] != totals or set(ends) != set(events) or set(facts["node_counts"]) != set(events):
            raise ValueError("Native object operation coverage is incomplete")
        for identity, fields in events.items():
            end = ends[identity]
            kind, selected = fields[1], int(fields[5])
            selection = facts["selections"].get(identity)
            if selection:
                if selected and selected != int(selection[0]):
                    raise ValueError("Object operation changes its selected procedure")
                selected = int(selection[0])
            if selected and kind not in ("generated-member-body", "generated-member-declaration"):
                expected = "destructor" if kind == "destructor-call" else "constructor"
                selected_owner = model.symbols[int(fields[4])][5] if kind == "base-initializer-source" else fields[3]
                if model.signatures.get(selected, [""] * 3)[2] != expected or model.symbols[selected][11] != selected_owner:
                    raise ValueError("Object operation selected another special member")
            choices = facts["choices"][identity]
            names = [row[0] for row in choices]
            required_choice = {"base-initialization": "base-init", "destructor-call": "dispatch",
                               "generated-assignment-field": "copy-route"}.get(kind)
            if required_choice and names.count(required_choice) != 1:
                raise ValueError("Object operation loses its native route choice")
            if kind == "destructor-call" and fields[6] == "1" and ["dispatch", "direct"] not in choices:
                raise ValueError("A forced base destructor call invents virtual dispatch")
            if kind == "implicit-construction" and names.count("access") != bool(int(fields[5])):
                raise ValueError("Implicit construction loses or invents an access check")
            if kind == "field-initialization" and int(fields[6]) == 0 and names.count("field-init") != 1:
                raise ValueError("Default field initialization loses its native policy")
            if kind == "vptr-initialization" and names.count("vptr-table") != (end[0] == "1"):
                raise ValueError("Vptr initialization invents or loses its actual table")
            if kind in ("union-initialization", "generated-union-copy"):
                members = [row[1] for row in choices if row[0] == "union-member"]
                if not members or members[0] != fields[4] or len(set(members)) != len(members) or names.count("union-bytes") != 1:
                    raise ValueError("Merged union operation loses native members or extent")
            if kind == "constructor-chain" and (not selection or not selected):
                raise ValueError("Constructor chaining lacks its actual selected call")
            if kind in ("default-constructor-call", "destructor-call", "reused-construction") and end[0] == "1" and not selected:
                raise ValueError("A constructed call lacks its selected special member")
            if kind == "implicit-construction" and end[2] == "shallow" and selected:
                raise ValueError("Shallow materialization invents a constructor")
            if kind in ("default-constructor-call", "destructor-call", "reused-construction") or kind == "implicit-construction" and selected:
                if end[0] == "1" and node_kinds[int(end[1])] != "call":
                    raise ValueError("Special member builder did not return its native call root")
            if kind == "default-constructor-call" and (bool(selected) != (end[2] == "built")):
                raise ValueError("Default construction invents or loses its actual selected call")
            if kind == "implicit-construction":
                if int(fields[7]) and (selected or end[2] != "error"):
                    raise ValueError("Failed constructor selection claims a valid materialization")
                if not int(fields[7]) and not selected and end[2] != "shallow":
                    raise ValueError("Unselected materialization claims a constructor call")
            if kind == "field-exclusion" and end[2] != "skipped":
                raise ValueError("An excluded field claims an executable object action")
            if kind == "generated-member-body" and int(fields[4]) not in bodies:
                raise ValueError("Generated body lacks its actual procedure phase")
            projected = facts["projections"][identity]
            if facts["node_counts"][identity] != len(projected) or projected and end[0] != "1" and (not selection or selection[1] == "0"):
                raise ValueError("Object operation invents or loses retained root links")
            if selected and kind in ("default-constructor-call", "destructor-call", "reused-construction"):
                for node_id in projected:
                    node = model.nodes[node_id]
                    if node_kinds[int(node[4])] == "call" and int(node[9]) != selected and static_targets.get(node_id) != selected:
                        raise ValueError("Object operation root calls another selected procedure")

# end of semantic_object_operations.py
