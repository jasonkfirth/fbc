"""Project: FreeBASIC semantic sidecar
File: semantic_lifetimes.py
Purpose: Validate normal lexical teardown groups and retained cleanup actions.
Responsibilities: Native ownership, order, insertion choices and complete closure.
This file intentionally does NOT infer destruction from a source type spelling.
"""

from collections import Counter, defaultdict

FEATURE = "native-lexical-cleanup"
PREFIXES = ("native-lexical-cleanup-group:", "native-lexical-cleanup:",
            "native-lexical-cleanup-end:", "native-lexical-cleanup-counts:")
LIMIT = 1000000


def validate_lifetimes(model, number, subject_modules):
    groups, actions, endings, nodes = (defaultdict(dict) for _ in range(4))
    closures = {}
    ordinals = defaultdict(list)
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Lexical cleanup integer is not canonical or bounded")
        return result

    def symbol(value, classes):
        identity = integer(value, 1)
        native = model.symbols.get(identity)
        if (native is None or native[3] not in classes
                or subject_modules.get(("symbol", identity)) != module):
            raise ValueError("Lexical cleanup symbol has another class or module")
        return identity

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not (row[3].startswith(PREFIXES) or row[3] == "native-lexical-cleanup"):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available" or module in closures:
            raise ValueError("Lexical cleanup is unavailable or closed")
        if row[3] == "native-lexical-cleanup":
            identity, action = integer(row[2], 1), integer(row[4], 1, LIMIT)
            if row[1] != "node" or identity in nodes[module]:
                raise ValueError("Lexical cleanup node is repeated or has another domain")
            nodes[module][identity] = action
            continue
        owner = symbol(row[2], ("8",))
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Lexical cleanup receipt lacks its module namespace")
        parts = row[3].split(":")
        if len(parts) != 2:
            raise ValueError("Malformed lexical cleanup key")
        family, identity = parts[0], integer(parts[1], 1, LIMIT)
        fields = tuple(row[4].split("\t"))
        if family == "native-lexical-cleanup-counts":
            if identity != module or len(fields) != 3:
                raise ValueError("Malformed lexical cleanup count closure")
            closures[module] = tuple(integer(value, 0, LIMIT) for value in fields)
        elif family == "native-lexical-cleanup-group":
            if identity != len(groups[module]) + 1 or len(fields) != 5:
                raise ValueError("Lexical cleanup group order is incomplete")
            symbol(fields[0], ("3", "15"))
            symbol(fields[1], ("3",))
            statement, source, context = map(integer, fields[2:])
            if statement and (statement not in model.statements or model.statements[statement][7] != str(module)):
                raise ValueError("Lexical cleanup group has a foreign statement")
            if source and (source not in model.source_contexts or model.source_contexts[source][4] != str(module)):
                raise ValueError("Lexical cleanup group has a foreign source")
            if context and model.configurations.get(context) != module:
                raise ValueError("Lexical cleanup group has a foreign configuration")
            groups[module][identity] = fields
        elif family == "native-lexical-cleanup":
            if identity != len(actions[module]) + 1 or len(fields) != 4:
                raise ValueError("Lexical cleanup action order is incomplete")
            group, ordinal = integer(fields[0], 1, LIMIT), integer(fields[1], 1, LIMIT)
            header = groups[module].get(group)
            if header is None or group in endings[module]:
                raise ValueError("Lexical cleanup action has no open group")
            variable = symbol(fields[2], ("1",))
            if (model.symbols[variable][11] != header[0]
                    or model.properties["symbol", variable].get("automatic-cleanup") != "1"
                    or fields[3] not in ("0", "1")):
                raise ValueError("Lexical cleanup has borrowed storage or another owner")
            ordinals[module, group].append(ordinal)
            actions[module][identity] = fields
        else:
            if identity not in groups[module] or identity in endings[module] or len(fields) != 1:
                raise ValueError("Lexical cleanup group ending is repeated or dangling")
            endings[module][identity] = integer(fields[0], 0, LIMIT)

    phase_owners = {int(row[1]): int(row[2]) for row in model.records["PH"]}
    node_phases = {int(row[1]): int(row[2]) for row in model.records["NP"]}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        if capabilities.get("native-block-structure") != "available":
            raise ValueError("Lexical cleanup lacks native block coverage")
        if closures.get(module) != (len(groups[module]), len(actions[module]), len(nodes[module])):
            raise ValueError("Lexical cleanup count closure is incomplete")
        if set(groups[module]) != set(endings[module]):
            raise ValueError("Lexical cleanup group endings are incomplete")
        blocks = model.native_blocks[module]
        expected = {int(fields[0]): int(fields[0]) for fields in blocks["native-procedure-block"].values()}
        expected.update({int(fields[0]): int(fields[2]) for fields in blocks["native-scope-block"].values()})
        observed = [int(fields[0]) for fields in groups[module].values()]
        if len(observed) != len(set(observed)) or set(observed) != set(expected):
            raise ValueError("Lexical teardown groups differ from native block owners")
        for identity, fields in groups[module].items():
            if int(fields[1]) != expected[int(fields[0])]:
                raise ValueError("Lexical cleanup group belongs to another procedure")
            if ordinals[module, identity] != list(range(1, endings[module][identity] + 1)):
                raise ValueError("Lexical cleanup order differs from its native group ending")
        retained = Counter()
        for identity, selected in nodes[module].items():
            fields = actions[module].get(selected)
            if identity not in model.nodes or fields is None or subject_modules[("node", identity)] != module:
                raise ValueError("Lexical cleanup node has no retained node or selected action")
            group = groups[module][int(fields[0])]
            if phase_owners.get(node_phases.get(identity)) != int(group[1]):
                raise ValueError("Lexical cleanup node belongs to another procedure")
            if "native-exit-cleanup" in model.properties["node", identity]:
                raise ValueError("Cleanup node has both exit and normal teardown owners")
            retained[selected] += 1
        if any(retained[identity] != int(fields[3]) for identity, fields in actions[module].items()):
            raise ValueError("Lexical cleanup roots differ from actual insertion choices")
    model.lexical_lifetimes = {module: {"groups": groups[module], "actions": actions[module],
                                        "ends": endings[module], "nodes": nodes[module]}
                               for module in groups}

# end of semantic_lifetimes.py
