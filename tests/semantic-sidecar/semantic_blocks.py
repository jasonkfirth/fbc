"""Project: FreeBASIC semantic sidecar
File: semantic_blocks.py
Purpose: Validate native block boundaries and scope-exit ownership.
Responsibilities: Parent closure, actual branch targets and selected cleanup order.
This file intentionally does NOT infer exits from lowered branch spelling.
"""

from collections import defaultdict

FEATURE = "native-block-structure"
PROVENANCE_FEATURE = "native-block-provenance"
FAMILIES = ("native-procedure-block", "native-scope-block", "native-scope-exit", "native-exit-cleanup")
PREFIXES = tuple(name + ":" for name in FAMILIES) + ("native-scope-exit-end:", "native-block-counts:",
                                                       "native-block-origin:", "native-block-provenance-counts:")
PREFIXES += ("native-cleanup-insertion:",)
LIMIT = 1000000


def validate_blocks(model, number, subject_modules):
    observations = defaultdict(lambda: defaultdict(dict))
    closures, exit_ends = {}, defaultdict(dict)
    origins, provenance_closures, cleanup_nodes = defaultdict(dict), {}, defaultdict(dict)
    insertions = defaultdict(dict)
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Native block integer is not canonical or bounded")
        return result

    def symbol(value, classes):
        identity = integer(value, 1)
        native = model.symbols.get(identity)
        if (native is None or native[3] not in classes
                or subject_modules.get(("symbol", identity)) != module):
            raise ValueError("Native block symbol has another class or module")
        return identity

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] == "K" and row[3] == "native-scope-exit":
            if row[1] != "node" or model.capabilities.get(module, {}).get(FEATURE) != "available":
                raise ValueError("Native branch exit link lacks its node domain or capability")
            integer(row[4], 1, LIMIT)
            continue
        if row[0] == "K" and row[3] == "native-exit-cleanup":
            if row[1] != "node" or model.capabilities.get(module, {}).get(PROVENANCE_FEATURE) != "available":
                raise ValueError("Native cleanup node lacks its domain or provenance capability")
            identity = integer(row[2], 1)
            if identity in cleanup_nodes[module] or module in provenance_closures:
                raise ValueError("Native cleanup node is repeated or follows closure")
            cleanup_nodes[module][identity] = integer(row[4], 1, LIMIT)
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Native block observation lacks its capability")
        owner = symbol(row[2], ("8",))
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Native block observation lacks its module namespace")
        pieces = row[3].split(":")
        if len(pieces) != 2:
            raise ValueError("Malformed native block key")
        family, identity = pieces[0], integer(pieces[1], 1, LIMIT)
        fields = tuple(row[4].split("\t"))
        if family in ("native-block-origin", "native-block-provenance-counts", "native-cleanup-insertion"):
            if (model.capabilities.get(module, {}).get(PROVENANCE_FEATURE) != "available"
                    or module in provenance_closures):
                raise ValueError("Native block provenance is unavailable or closed")
            if family == "native-block-provenance-counts":
                if identity != module or len(fields) != 2:
                    raise ValueError("Malformed native block provenance closure")
                provenance_closures[module] = tuple(integer(value, 0, LIMIT) for value in fields)
                continue
            if family == "native-cleanup-insertion":
                if identity in insertions[module] or len(fields) != 1 or fields[0] not in ("0", "1"):
                    raise ValueError("Invalid or repeated native cleanup insertion")
                insertions[module][identity] = fields[0]
                continue
            if len(fields) != 5 or identity != len(origins[module]) + 1 or fields[0] not in ("procedure", "scope"):
                raise ValueError("Malformed native block origin")
            symbol(fields[1], ("3",) if fields[0] == "procedure" else ("15",))
            statement, source, context = map(integer, fields[2:])
            if statement and (statement not in model.statements or model.statements[statement][7] != str(module)):
                raise ValueError("Native block origin has a foreign source statement")
            if source and (source not in model.source_contexts or model.source_contexts[source][4] != str(module)):
                raise ValueError("Native block origin has a foreign source occurrence")
            if context and model.configurations.get(context) != module:
                raise ValueError("Native block origin has a foreign configuration")
            origins[module][identity] = fields
            continue
        if module in closures:
            raise ValueError("Native block observation follows module closure")
        if family == "native-block-counts":
            if identity != module or len(fields) != len(FAMILIES):
                raise ValueError("Malformed native block closure")
            closures[module] = tuple(integer(value, 0, LIMIT) for value in fields)
            continue
        if family == "native-scope-exit-end":
            if identity in exit_ends[module] or len(fields) != 1:
                raise ValueError("Repeated or malformed scope exit closure")
            exit_ends[module][identity] = integer(fields[0], 0, LIMIT)
            continue
        bucket = observations[module][family]
        if identity != len(bucket) + 1:
            raise ValueError("Native block observation order is missing or repeated")
        if family == "native-procedure-block":
            if len(fields) != 6:
                raise ValueError("Malformed procedure block")
            symbol(fields[0], ("3",))
            symbol(fields[1], ("7",))
            symbol(fields[2], ("7",))
            if integer(fields[4]) < integer(fields[3]) or fields[5] not in ("0", "1"):
                raise ValueError("Invalid procedure interval or main flag")
            if fields[1] == fields[2]:
                raise ValueError("Procedure entry and exit targets are the same")
        elif family == "native-scope-block":
            if len(fields) != 5:
                raise ValueError("Malformed scope block")
            symbol(fields[0], ("15",))
            symbol(fields[1], ("3", "15"))
            symbol(fields[2], ("3",))
            if integer(fields[4]) < integer(fields[3]):
                raise ValueError("Reversed scope interval")
        elif family == "native-scope-exit":
            if len(fields) != 7:
                raise ValueError("Malformed native scope exit")
            symbol(fields[0], ("3",))
            symbol(fields[1], ("3", "15"))
            symbol(fields[2], ("7",))
            for value in fields[3:]:
                integer(value)
            if fields[6] != "0":
                statement = model.statements.get(int(fields[6]))
                if statement is None or statement[7] != str(module):
                    raise ValueError("Native scope exit has a foreign source statement")
        else:
            if len(fields) != 4:
                raise ValueError("Malformed exit cleanup")
            integer(fields[0], 1, LIMIT)
            integer(fields[1], 1, LIMIT)
            variable = symbol(fields[2], ("1",))
            symbol(fields[3], ("3", "15"))
            if (model.symbols[variable][11] != fields[3]
                    or model.properties["symbol", variable].get("automatic-cleanup") != "1"):
                raise ValueError("Exit cleanup has borrowed storage or another native owner")
        bucket[identity] = fields

    procedure_targets = {int(row[2]): int(row[4]) for row in model.relations("procedure-exit-label")}
    procedure_phases = {int(row[2]) for row in model.records["PH"]}
    node_phases = {int(row[1]): int(row[2]) for row in model.records["NP"]}
    phase_owners = {int(row[1]): int(row[2]) for row in model.records["PH"]}
    module_procedures, module_nodes = defaultdict(set), defaultdict(list)
    for identity in procedure_phases:
        module_procedures[subject_modules[("symbol", identity)]].add(identity)
    for identity, node in model.nodes.items():
        module_nodes[subject_modules[("node", identity)]].append((identity, node))
    branch_targets = defaultdict(set)
    for row in model.relations("branch-target"):
        if row[1] == "node":
            branch_targets[int(row[2])].add(int(row[4]))

    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        facts = observations[module]
        if closures.get(module) != tuple(len(facts[family]) for family in FAMILIES):
            raise ValueError("Native block count closure is incomplete")
        procedures = {int(fields[0]): fields for fields in facts[FAMILIES[0]].values()}
        scopes = {int(fields[0]): fields for fields in facts[FAMILIES[1]].values()}
        if len(procedures) != len(facts[FAMILIES[0]]) or len(scopes) != len(facts[FAMILIES[1]]):
            raise ValueError("Repeated native procedure or scope block")
        expected = module_procedures[module]
        if set(procedures) != expected:
            raise ValueError("Procedure block snapshots differ from exported body phases")
        for procedure, fields in procedures.items():
            if procedure_targets.get(procedure) != int(fields[2]):
                raise ValueError("Procedure block exit differs from its native relation")

        # Resolve each native parent chain once. This also detects cycles
        # without recursive Python calls or quadratic deep-scope traversal.
        scope_procedures = {}
        for scope in scopes:
            path, active = [], set()
            current = scope
            while current not in scope_procedures:
                if current in active or current not in scopes:
                    raise ValueError("Native scope parent chain is cyclic or incomplete")
                active.add(current)
                path.append(current)
                parent = int(scopes[current][1])
                if parent in procedures:
                    owner = parent
                    break
                current = parent
            else:
                owner = scope_procedures[current]
            for current in path:
                if int(scopes[current][2]) != owner:
                    raise ValueError("Native scope belongs to another procedure")
                scope_procedures[current] = owner

        exits = facts[FAMILIES[2]]
        if set(exit_ends[module]) != set(exits):
            raise ValueError("Native scope exit closure is incomplete")
        cleanup_ordinals = defaultdict(list)
        for fields in facts[FAMILIES[3]].values():
            identity, ordinal, _, owner = map(int, fields)
            source = exits.get(identity)
            if source is None:
                raise ValueError("Cleanup lacks its native scope exit")
            if (scope_procedures.get(owner, owner) != int(source[0])):
                raise ValueError("Cleanup belongs to another exiting procedure")
            cleanup_ordinals[identity].append(ordinal)
        for identity, fields in exits.items():
            proc, owner = map(int, fields[:2])
            if proc not in procedures or scope_procedures.get(owner, owner) != proc:
                raise ValueError("Scope exit belongs to another procedure")
            if cleanup_ordinals[identity] != list(range(1, exit_ends[module][identity] + 1)):
                raise ValueError("Scope exit cleanup order or count is incomplete")

        if capabilities.get(PROVENANCE_FEATURE) == "available":
            expected_origins = {("procedure", identity) for identity in procedures}
            expected_origins.update(("scope", identity) for identity in scopes)
            actual_origins = [(fields[0], int(fields[1])) for fields in origins[module].values()]
            if len(actual_origins) != len(set(actual_origins)) or set(actual_origins) != expected_origins:
                raise ValueError("Native block origins differ from block snapshots")
            if provenance_closures.get(module) != (len(origins[module]), len(cleanup_nodes[module])):
                raise ValueError("Native block provenance count closure is incomplete")
            if set(insertions[module]) != set(facts[FAMILIES[3]]):
                raise ValueError("Native cleanup insertion coverage is incomplete")
            selected_nodes = defaultdict(int)
            for identity, cleanup in cleanup_nodes[module].items():
                selected = facts[FAMILIES[3]].get(cleanup)
                if identity not in model.nodes or selected is None:
                    raise ValueError("Native cleanup node lacks its retained node or selected action")
                source = exits[int(selected[0])]
                if (subject_modules[("node", identity)] != module
                        or phase_owners.get(node_phases.get(identity)) != int(source[0])):
                    raise ValueError("Native cleanup node belongs to another exiting procedure")
                selected_nodes[cleanup] += 1
            if any(selected_nodes[cleanup] != int(inserted) for cleanup, inserted in insertions[module].items()):
                raise ValueError("Retained cleanup roots differ from native insertion choices")

        for identity, node in module_nodes[module]:
            kind = model.properties["node", identity].get("kind")
            if kind in ("scope-begin", "scope-end") and int(node[9]) not in scopes:
                raise ValueError("Native scope node lacks its block snapshot")
            source = model.properties["node", identity].get("native-scope-exit")
            if source is None:
                continue
            source = integer(source, 1, LIMIT)
            fields = exits.get(source)
            if (kind != "branch" or fields is None or branch_targets[identity] != {int(fields[2])}
                    or phase_owners.get(node_phases.get(identity)) != int(fields[0])):
                raise ValueError("Lowered branch differs from its native scope exit")
    model.native_blocks = dict(observations)
    model.native_block_origins = dict(origins)
    model.native_cleanup_nodes = dict(cleanup_nodes)

# end of semantic_blocks.py
