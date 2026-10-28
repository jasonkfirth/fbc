"""Project: FreeBASIC semantic sidecar
File: semantic_effects.py
Purpose: Validate compiler-owned effect contracts and preserved uncertainty.
Responsibilities: Runtime provenance, named masks, selected calls and IR closure.
This file intentionally does NOT infer purity or alias sets from external names.
"""

from collections import defaultdict

FEATURE = "native-effect-contracts"
DOMAINS = ("unknown", "memory", "math", "string", "unicode", "array", "file", "print", "console",
           "graphics", "sound", "error", "profile", "system", "gosub", "data", "object", "serialization")
EFFECTS = ("unknown", "allocate", "release", "argument-read", "argument-write", "runtime-read", "runtime-write",
           "external-io", "synchronization", "error-handling", "profiling", "process-exit", "code-execution", "computation", "control-transfer")
LIMIT = 1000000


def validate_effects(model, number, subject_modules):
    modules = {}
    declarations = {}
    expected_declarations = defaultdict(list)
    module = 0
    live_types = {}
    flags = {}
    for identity, definitions in model.flag_definitions.items():
        flags[identity] = {name: int(mask) for group, name, mask, _ in definitions.values() if group == "status"}

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Effect integer is not canonical or bounded")
        return result

    def symbol(value):
        identity = integer(value, 1)
        row = model.symbols.get(identity)
        if row is None or row[3] != "3" or subject_modules.get(("symbol", identity)) != module:
            raise ValueError("Effect contract has another procedure or module")
        return identity

    def contract(fields, selected, indirect=False):
        if len(fields) != 7:
            raise ValueError("Malformed native effect contract")
        origin, domain, raw, names, certainty, options, aliases = fields
        mask = integer(raw, 1, (1 << len(EFFECTS)) - 1)
        integer(options)
        expected_names = ",".join(name for bit, name in enumerate(EFFECTS) if mask & (1 << bit))
        if domain not in DOMAINS or names != expected_names or aliases != "unknown-aliases":
            raise ValueError("Effect mask invents vocabulary or an alias set")
        if certainty != ("unknown-remainder" if mask & 1 else "known-potential"):
            raise ValueError("Effect contract loses its unknown remainder")
        declaration = live_types.get(selected)
        if declaration is None:
            raise ValueError("Effect contract lacks its live type snapshot")
        if indirect:
            if fields != ["indirect-target", "unknown", "1", "unknown", "unknown-remainder", "0", "unknown-aliases"]:
                raise ValueError("Indirect call invents a resolved effect contract")
        elif origin == "runtime-registration":
            if domain == "unknown" or declaration[18] != "runtime" or model.signatures[selected][2] == "procedure-pointer":
                raise ValueError("Effect contract invents runtime registration provenance")
            if mask == 8192 and domain != "math" and domain != "object":
                raise ValueError("Computation-only contract has another runtime domain")
        elif fields != ["unknown-callee", "unknown", "1", "unknown", "unknown-remainder", "0", "unknown-aliases"]:
            raise ValueError("Unregistered declaration invents known effects")
        return fields

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] == "T":
            live_types[int(row[1])] = row
            if row[3] == "procedure":
                expected_declarations[module].append(int(row[1]))
        if row[0] != "K" or not row[3].startswith(("effect-declaration:", "effect-call:", "effect-operation:", "effect-domain:", "effect-definition:", "effect-counts:")):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Effect record has unavailable coverage")
        state = modules.setdefault(module, {"declarations": {}, "calls": {}, "operations": {}, "domains": {}, "definitions": {}, "closure": None})
        if state["closure"] is not None:
            raise ValueError("Effect record follows its module closure")
        parts = row[3].split(":")
        if len(parts) != 2:
            raise ValueError("Malformed effect identity")
        identity = integer(parts[1], 0, LIMIT)
        family = parts[0][len("effect-"):]
        fields = row[4].split("\t")
        if family == "declaration":
            selected = symbol(row[2])
            if row[1] != "symbol" or identity != len(state["declarations"]) + 1 or len(fields) != 9:
                raise ValueError("Effect declaration order or fields are invalid")
            contract(fields[:7], selected)
            stats = int(live_types[selected][12])
            if fields[7:] != [str(int(bool(stats & flags[module]["call-cloneable"]))), str(int(bool(stats & flags[module]["throwable"])))] :
                raise ValueError("Effects invent native cloneability or error flags")
            state["declarations"][identity] = selected, fields
            declarations[selected] = fields[:7]
            continue
        owner = integer(row[2], 1)
        root = model.symbols.get(owner)
        if row[1] != "symbol" or root is None or root[3] != "8" or root[11] != "0" or subject_modules.get(("symbol", owner)) != module:
            raise ValueError("Effect record lacks its module namespace")
        if family == "call":
            if identity != len(state["calls"]) + 1 or len(fields) != 12:
                raise ValueError("Malformed or unordered effect call")
            graph_id, action_id, load_id = (integer(value, 0, LIMIT) for value in fields[:3])
            selected = symbol(fields[3])
            indirect = integer(fields[4], 0, 1)
            graph = model.native_ir_flow[module]["graphs"].get(graph_id)
            action = graph["actions"].get(action_id) if graph else None
            if action is None or action[0] != "call" or action[2] != fields[2] or action[3] != fields[4] or action[5] != fields[3]:
                raise ValueError("Effect call is not its actual native IR call")
            contract(fields[5:], selected, bool(indirect))
            if not indirect and declarations.get(selected) != fields[5:]:
                raise ValueError("Direct call changes its selected declaration contract")
            state["calls"][identity] = fields
        elif family == "operation":
            if identity != len(state["operations"]) + 1 or len(fields) != 8:
                raise ValueError("Malformed or unordered effect operation")
            graph_id, action_id, load_id = (integer(value, 0, LIMIT) for value in fields[:3])
            graph = model.native_ir_flow[module]["graphs"].get(graph_id)
            action = graph["actions"].get(action_id) if graph else None
            if action is None or action[0] == "call" or action[0] != fields[3] or action[2] != fields[2] or action[3] != fields[4]:
                raise ValueError("Effect operation is not its actual native IR action")
            expected = "unknown-memory-registers-control" if fields[3] == "assembly" else "unknown"
            if fields[6:] != [expected, "unknown-aliases"]:
                raise ValueError("Opaque IR/assembly operation invents purity or alias precision")
            state["operations"][identity] = fields
        elif family == "domain":
            if identity in state["domains"] or identity >= len(DOMAINS) or fields != [DOMAINS[identity]]:
                raise ValueError("Effect domain vocabulary is repeated or changed")
            state["domains"][identity] = fields
        elif family == "definition":
            if identity in state["definitions"] or identity >= len(EFFECTS) or fields != [str(1 << identity), EFFECTS[identity]]:
                raise ValueError("Effect mask vocabulary is repeated or changed")
            state["definitions"][identity] = fields
        elif family == "counts":
            if identity != module or len(fields) != 3:
                raise ValueError("Malformed effect module closure")
            state["closure"] = [integer(value, 0, LIMIT) for value in fields]
        else:
            raise ValueError("Unknown effect observation family")
    model.effect_contracts = modules
    for module in range(1, len(model.records["M"]) + 1):
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            continue
        state = modules.get(module)
        if (state is None or state["closure"] != [len(state[family]) for family in ("declarations", "calls", "operations")]
                or set(state["domains"]) != set(range(len(DOMAINS))) or set(state["definitions"]) != set(range(len(EFFECTS)))):
            raise ValueError("Effect declaration or vocabulary coverage is incomplete")
        if [selected for selected, _ in state["declarations"].values()] != expected_declarations[module]:
            raise ValueError("Effect declarations lose an actual procedure snapshot")
        expected_calls, expected_operations = set(), set()
        for graph_id, graph in model.native_ir_flow[module]["graphs"].items():
            for action_id, action in graph["actions"].items():
                (expected_calls if action[0] == "call" else expected_operations).add((graph_id, action_id))
        actual_calls = [(int(row[0]), int(row[1])) for row in state["calls"].values()]
        actual_operations = [(int(row[0]), int(row[1])) for row in state["operations"].values()]
        if set(actual_calls) != expected_calls or len(actual_calls) != len(expected_calls) or set(actual_operations) != expected_operations or len(actual_operations) != len(expected_operations):
            raise ValueError("Effect observations lose actual IR calls or operations")

# end of semantic_effects.py
