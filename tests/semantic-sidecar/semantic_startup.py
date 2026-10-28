"""Project: FreeBASIC semantic sidecar
File: semantic_startup.py
Purpose: Validate actual startup registrations and guarded instance paths.
Responsibilities: List/action order, wrapper ownership, phases and complete closure.
This file intentionally does NOT predict target loader order from export IDs.
"""

from collections import defaultdict
import re

FEATURE = "native-startup-lifecycle"
DECLARATION_FEATURE = "accepted-declarator-order"
LIMIT = 1000000
FAMILIES = ("startup-registration", "startup-priority-input", "global-instance", "static-instance",
            "static-instance-guard", "startup-instance-wrapper", "global-instance-action", "static-cleanup-body", "startup-emission")
PREFIXES = tuple(name + ":" for name in FAMILIES) + ("startup-lifecycle-counts:", "accepted-declarator-order:", "accepted-declarator-count:")


def validate_startup(model, number, subject_modules):
    observations = defaultdict(lambda: defaultdict(dict))
    closures = {}
    kind_ordinals = defaultdict(lambda: defaultdict(int))
    registration_kinds = defaultdict(set)
    accepted_priorities = defaultdict(list)
    emitted = {int(row[2]) for row in model.records["PH"] if row[3] == "pre-load" and row[4] == "1"}
    declaration_orders, declaration_closures = defaultdict(dict), {}
    declaration_groups, declaration_inputs = {}, {}
    expected_declarations = defaultdict(list)
    for row in model.records["K"]:
        owner_module = subject_modules.get((row[1], int(row[2])))
        if row[3].startswith("declaration-type-group:"):
            declaration_groups[owner_module, row[3].split(":")[1]] = row[2], row[4].split("\t")[0]
        elif row[3].startswith("declaration-type-input:"):
            _, group, item = row[3].split(":")
            key = owner_module, group, item
            declaration_inputs[key] = row[2], row[4].split("\t")
            expected_declarations[owner_module].append(key)
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Startup integer is not canonical or bounded")
        return result

    def symbol(value, raw_class, nullable=False):
        identity = integer(value)
        if not identity and nullable:
            return identity
        info = model.symbols.get(identity)
        if info is None or info[3] != raw_class or subject_modules.get(("symbol", identity)) != module:
            raise ValueError("Startup symbol is foreign or has another class")
        return identity

    def kind(value):
        if value not in ("constructor", "destructor"):
            raise ValueError("Unknown startup registration kind")
        return value

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        declaration = row[3].startswith(("accepted-declarator-order:", "accepted-declarator-count:"))
        if model.capabilities.get(module, {}).get(DECLARATION_FEATURE if declaration else FEATURE) != "available":
            raise ValueError("Startup observation has unavailable coverage")
        owner = symbol(row[2], "8")
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Startup observation lacks its module namespace owner")
        pieces = row[3].split(":")
        if len(pieces) != 2:
            raise ValueError("Startup observation has a malformed key")
        family, ordinal = pieces[0], integer(pieces[1], 1, LIMIT)
        fields = row[4].split("\t")
        if declaration:
            if family == "accepted-declarator-count":
                if ordinal != module or module in declaration_closures or len(fields) != 1:
                    raise ValueError("Accepted declarator order closure is repeated or foreign")
                declaration_closures[module] = integer(fields[0], 0, LIMIT)
                continue
            if module in declaration_closures or len(fields) != 6 or ordinal != len(declaration_orders[module]) + 1:
                raise ValueError("Accepted declarator sequence is malformed, closed or reordered")
            declared = integer(fields[0], 1)
            info = model.symbols.get(declared)
            if info is None or info[3] not in ("1", "2") or subject_modules.get(("symbol", declared)) != module:
                raise ValueError("Accepted declarator has another symbol class or module")
            parent = model.symbols.get(integer(fields[1], 1))
            if parent is None or parent[3] not in ("3", "8"):
                raise ValueError("Accepted declarator has another native group owner")
            symbol(fields[1], parent[3])
            group, item, statement, source = fields[2:]
            for value in fields[2:]:
                integer(value, 1)
            native = declaration_inputs.get((module, group, item))
            header = declaration_groups.get((module, group))
            if (native is None or header != (fields[1], statement) or native[0] != fields[1]
                    or native[1][0] != fields[0] or native[1][3] != source):
                raise ValueError("Accepted declarator order differs from its native group input")
            declaration_orders[module][ordinal] = tuple(fields)
            continue
        if family == "startup-lifecycle-counts":
            if ordinal != module or module in closures or len(fields) != len(FAMILIES):
                raise ValueError("Startup closure is repeated or foreign")
            closures[module] = [integer(value, 0, LIMIT) for value in fields]
            continue
        if module in closures:
            raise ValueError("Startup observation follows module closure")
        bucket = observations[module][family]
        if ordinal != len(bucket) + 1:
            raise ValueError("Startup observation order is missing or repeated")
        if family == "startup-registration":
            if len(fields) != 3:
                raise ValueError("Malformed startup registration")
            proc = symbol(fields[0], "3")
            selected_kind = kind(fields[1])
            kind_ordinals[module][selected_kind] += 1
            if integer(fields[2], 1, LIMIT) != kind_ordinals[module][selected_kind]:
                raise ValueError("Startup list append order is incomplete")
            registration_kinds[proc].add(selected_kind)
        elif family == "startup-priority-input":
            if len(fields) != 2:
                raise ValueError("Malformed accepted startup priority")
            proc = symbol(fields[0], "3")
            integer(fields[1], 0, 65535)
            if not registration_kinds[proc]:
                raise ValueError("Startup priority lacks its native registration")
            accepted_priorities[proc].append(fields[1])
        elif family == "global-instance":
            if len(fields) != 3:
                raise ValueError("Malformed global instance")
            symbol(fields[0], "1")
            for value in fields[1:]:
                integer(value, 0, 1)
        elif family == "static-instance":
            if len(fields) != 7:
                raise ValueError("Malformed static instance path")
            symbol(fields[0], "1")
            symbol(fields[1], "3")
            wrapper = symbol(fields[2], "3", nullable=True)
            initialized, cleanup, registered = (integer(value, 0, 1) for value in fields[3:6])
            if fields[6] not in ("initialization", "array-allocation"):
                raise ValueError("Unknown local static path purpose")
            if bool(wrapper) != bool(cleanup) or cleanup != registered:
                raise ValueError("Static cleanup invents a wrapper or unconditional registration")
            if fields[6] == "array-allocation" and (wrapper or cleanup or registered):
                raise ValueError("Static allocation path invents a cleanup registration")
        elif family == "static-instance-guard":
            if len(fields) != 5:
                raise ValueError("Malformed local static guard")
            identity = integer(fields[0], 1, LIMIT)
            symbol(fields[1], "1")
            symbol(fields[2], "7")
            if identity not in observations[module]["static-instance"] or fields[3:] != ["zero", "before-actions"]:
                raise ValueError("Static guard has another path or native condition")
        elif family == "startup-instance-wrapper":
            if len(fields) != 2:
                raise ValueError("Malformed global instance wrapper")
            proc = symbol(fields[0], "3")
            selected_kind = kind(fields[1])
            if selected_kind not in registration_kinds[proc]:
                raise ValueError("Generated instance wrapper lacks its startup registration")
        elif family == "global-instance-action":
            if len(fields) != 4:
                raise ValueError("Malformed global instance action")
            identity = integer(fields[0], 1, LIMIT)
            symbol(fields[1], "3")
            integer(fields[3], 1, LIMIT)
            if identity not in observations[module]["global-instance"] or fields[2] not in ("initialize", "cleanup"):
                raise ValueError("Global action has another instance or kind")
        elif family == "static-cleanup-body":
            if len(fields) != 3:
                raise ValueError("Malformed static cleanup body")
            symbol(fields[0], "1")
            symbol(fields[1], "3")
            symbol(fields[2], "3")
        elif family == "startup-emission":
            if len(fields) != 8:
                raise ValueError("Malformed selected startup emission")
            proc = symbol(fields[0], "3")
            selected_kind = kind(fields[1])
            backend = model.contexts[module - 1][5]
            stage = "c" if backend in ("gcc", "clang") else backend
            if fields[2] != stage or selected_kind not in registration_kinds[proc]:
                raise ValueError("Startup emission has another backend or registration kind")
            requested = integer(fields[4], 0, 65535)
            selected, placement, spelling = fields[5:]
            if selected and (re.fullmatch(r"[0-9]{1,20}", selected) is None or int(selected) > 65535):
                raise ValueError("Startup encoded priority is invalid or unbounded")
            if (not placement or not spelling or len(placement.encode("utf-8", errors="surrogateescape")) > 65536
                    or len(spelling.encode("utf-8", errors="surrogateescape")) > 65536 or "\0" in placement + spelling):
                raise ValueError("Startup emission text is unavailable or excessive")
            if stage == "c":
                if fields[3] in ("definition-attribute", "declaration-attribute"):
                    if placement != "function-attribute" or selected_kind not in spelling or selected != (str(requested) if requested else ""):
                        raise ValueError("C startup attribute contradicts its selected kind or priority")
                elif fields[3] != "array-entry" or selected_kind != "destructor" or not placement.startswith(".fini_array."):
                    raise ValueError("C startup emission invents another registration route")
            elif stage == "llvm":
                if (fields[3] != "array-entry" or placement != ("llvm.global_ctors" if selected_kind == "constructor" else "llvm.global_dtors")
                        or selected != str(requested or 65535) or "i32 " + selected not in spelling):
                    raise ValueError("LLVM startup array entry loses its selected priority or placement")
            elif stage == "gas":
                if (fields[3] != "section-request" or placement != ("constructor-section" if selected_kind == "constructor" else "destructor-section")
                        or selected != str(requested)):
                    raise ValueError("Assembler startup section request has another native choice")
            elif stage == "gas64":
                if fields[3] != "section-entry" or not placement.startswith((".ctors", ".dtors", ".init_array", ".fini_array")):
                    raise ValueError("x86-64 startup section has another native route")
            else:
                raise ValueError("Startup emission has an unsupported backend stage")
        else:
            raise ValueError("Unknown startup observation family")
        bucket[ordinal] = tuple(fields)

    model.startup_lifecycles = {}
    model.declarator_orders = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(DECLARATION_FEATURE) == "available":
            orders = declaration_orders[module]
            keys = [(module, fields[2], fields[3]) for fields in orders.values()]
            if declaration_closures.get(module) != len(orders) or keys != expected_declarations[module]:
                raise ValueError("Accepted declarator order coverage is incomplete")
            model.declarator_orders[module] = orders
        if capabilities.get(FEATURE) != "available":
            continue
        facts = observations[module]
        if module not in closures or closures[module] != [len(facts[family]) for family in FAMILIES]:
            raise ValueError("Startup lifecycle module coverage is incomplete")
        wrappers = {}
        for proc, selected_kind in facts["startup-instance-wrapper"].values():
            if proc in wrappers:
                raise ValueError("Global instance wrapper is repeated")
            wrappers[proc] = selected_kind
        actions = defaultdict(list)
        for identity, wrapper, selected_kind, ordinal in facts["global-instance-action"].values():
            expected = "constructor" if selected_kind == "initialize" else "destructor"
            if wrappers.get(wrapper) != expected or int(ordinal) != len(actions[selected_kind]) + 1:
                raise ValueError("Global instance action has another wrapper or native order")
            actions[selected_kind].append(int(identity))
        initializers = [identity for identity, fields in facts["global-instance"].items() if fields[1] == "1"]
        cleanups = [identity for identity, fields in reversed(list(facts["global-instance"].items())) if fields[2] == "1"]
        if actions["initialize"] != initializers or actions["cleanup"] != cleanups:
            raise ValueError("Global instance action coverage or forward/reverse order is incomplete")
        guards = {}
        for fields in facts["static-instance-guard"].values():
            identity = int(fields[0])
            if identity in guards:
                raise ValueError("Local static path repeats its guard")
            guards[identity] = fields
        bodies = defaultdict(list)
        for fields in facts["static-cleanup-body"].values():
            bodies[fields[2]].append(fields)
        expected_bodies = set()
        for identity, fields in facts["static-instance"].items():
            if (identity in guards) != (fields[3] == "1" or fields[5] == "1"):
                raise ValueError("Static path invents or loses its one-time guard")
            if fields[4] == "1" and int(fields[1]) in emitted:
                expected_bodies.add(fields[2])
                if bodies[fields[2]] != [fields[:3]]:
                    raise ValueError("Static wrapper body loses its instance or parent ownership")
        if set(bodies) != expected_bodies:
            raise ValueError("Static cleanup body invents unregistered or un-emitted ownership")
        for proc, priorities in accepted_priorities.items():
            if subject_modules["symbol", proc] != module:
                continue
            if model.properties["symbol", proc].get("startup-priority") != priorities[-1]:
                raise ValueError("Final procedure priority differs from the last accepted input")
        model.startup_lifecycles[module] = {family: facts[family] for family in FAMILIES}

# end of semantic_startup.py
