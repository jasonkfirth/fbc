"""Project: FreeBASIC semantic sidecar
File: semantic_temporaries.py
Purpose: Replay native destructor-list lifetime observations without executing them.
Responsibilities: Registration identities, cookie reuse, references, flushes and branches.
This file intentionally does NOT infer runtime execution from a retained flush action.
"""

FEATURE = "native-temporary-lifetime-events"
NODE_FEATURE = "native-temporary-cleanup-nodes"
PREFIXES = ("native-temp-event:", "native-temp-counts:", "native-temp-action-nodes:", "native-temp-projection-counts:")
LIMIT = 1000000


def validate_temporaries(model, number, subject_modules):
    modules = {}
    module = 0

    def fresh():
        return {"events": {}, "registrations": {}, "live": {}, "scopes": [], "scope_events": {},
                "flush_stack": [], "flushes": {}, "conditionals": {}, "epoch": 1,
                "closed": False, "pending_flush_removals": set(), "nodes": {}, "projections": {}, "projection_counts": None}

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Temporary lifetime scalar is not canonical or bounded")
        return result

    def symbol(value, classes, nullable=False):
        identity = integer(value)
        if identity == 0 and nullable:
            return identity
        native = model.symbols.get(identity)
        if (native is None or native[3] not in classes
                or subject_modules.get(("symbol", identity)) != module):
            raise ValueError("Temporary lifetime symbol has another class or module")
        return identity

    def selected(state, value):
        generation = integer(value, 1, LIMIT)
        item = state["live"].get(generation)
        if item is None:
            raise ValueError("Temporary transition has no live registration")
        return generation, item

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not (row[3].startswith(PREFIXES) or row[3] == "native-temp-cleanup"):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Temporary lifetime event lacks its capability")
        state = modules.setdefault(module, fresh())
        if state["closed"]:
            raise ValueError("Temporary lifetime event follows module closure")
        projection = row[3].startswith(("native-temp-action-nodes:", "native-temp-projection-counts:")) or row[3] == "native-temp-cleanup"
        if projection and (model.capabilities[module].get(NODE_FEATURE) != "available" or state["projection_counts"] is not None):
            raise ValueError("Temporary cleanup projection is unavailable or closed")
        if row[3] == "native-temp-cleanup":
            node, action = integer(row[2], 1), integer(row[4], 1, LIMIT)
            if row[1] != "node" or node in state["nodes"]:
                raise ValueError("Temporary cleanup node is repeated or has another domain")
            state["nodes"][node] = action
            continue
        owner = symbol(row[2], ("8",))
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Temporary lifetime event lacks its module namespace")
        parts = row[3].split(":")
        if len(parts) != 2:
            raise ValueError("Malformed temporary lifetime key")
        family, identity = parts[0], integer(parts[1], 1, LIMIT)
        fields = tuple(row[4].split("\t"))
        if family == "native-temp-action-nodes":
            if identity in state["projections"] or len(fields) != 1:
                raise ValueError("Temporary action projection is repeated or malformed")
            state["projections"][identity] = integer(fields[0], 0, LIMIT)
            continue
        if family == "native-temp-projection-counts":
            if identity != module or len(fields) != 2:
                raise ValueError("Malformed temporary projection closure")
            state["projection_counts"] = tuple(integer(value, 0, LIMIT) for value in fields)
            continue
        if family == "native-temp-counts":
            if identity != module or len(fields) != 6:
                raise ValueError("Malformed temporary lifetime closure")
            counts = tuple(integer(value, 0, LIMIT) for value in fields)
            expected = (len(state["events"]), len(state["registrations"]), len(state["flushes"]),
                        len(state["conditionals"]), len(state["live"]), len(state["scopes"]))
            if counts != expected or state["flush_stack"] or state["pending_flush_removals"]:
                raise ValueError("Temporary lifetime event closure is incomplete")
            state["closed"] = True
            continue
        if state["projection_counts"] is not None:
            raise ValueError("Temporary event follows projection closure")
        if identity != len(state["events"]) + 1 or not fields:
            raise ValueError("Temporary event order is incomplete")
        kind, values = fields[0], fields[1:]
        if kind == "add":
            if len(values) != 10:
                raise ValueError("Malformed temporary registration")
            generation = integer(values[0], 1, LIMIT)
            storage = symbol(values[1], ("1",))
            cookie, references = integer(values[2], -1), integer(values[3], 0, LIMIT)
            scope, epoch, statement, source, context = map(integer, values[4:9])
            symbol(values[9], ("3",), nullable=True)
            if generation in state["registrations"] or references or cookie < 0 or epoch != state["epoch"]:
                raise ValueError("Invalid or repeated temporary registration")
            if scope != (state["scopes"][-1] if state["scopes"] else 0):
                raise ValueError("Temporary registration has another scope occurrence")
            if cookie != (state["scope_events"][scope][1] if scope else 0):
                raise ValueError("Temporary registration has another native cookie")
            if statement and (statement not in model.statements or model.statements[statement][7] != str(module)):
                raise ValueError("Temporary registration has a foreign statement")
            if source and (source not in model.source_contexts or model.source_contexts[source][4] != str(module)):
                raise ValueError("Temporary registration has a foreign source")
            if context and model.configurations.get(context) != module:
                raise ValueError("Temporary registration has a foreign configuration")
            item = {"symbol": storage, "cookie": cookie, "references": 0, "scope": scope, "event": identity}
            state["registrations"][generation] = item.copy()
            state["live"][generation] = item
        elif kind in ("reference-add", "reference-remove"):
            if len(values) != 3:
                raise ValueError("Malformed temporary reference transition")
            _, item = selected(state, values[0])
            cookie, references = integer(values[1], -1), integer(values[2], 0, LIMIT)
            expected = item["references"] + (1 if kind == "reference-add" else -1)
            if cookie != item["cookie"] or references != expected:
                raise ValueError("Temporary reference transition disagrees with native state")
            item["references"] = references
        elif kind == "rescope":
            if len(values) != 3:
                raise ValueError("Malformed temporary rescope")
            _, item = selected(state, values[0])
            old, new = integer(values[1], -1), integer(values[2], -1)
            if old != item["cookie"]:
                raise ValueError("Temporary rescope starts from another cookie")
            item["cookie"] = new
        elif kind == "remove":
            if len(values) != 2:
                raise ValueError("Malformed temporary removal")
            generation, item = selected(state, values[0])
            reason = values[1]
            if reason == "flush":
                if generation not in state["pending_flush_removals"]:
                    raise ValueError("Temporary flush removal lacks its selected action")
                state["pending_flush_removals"].remove(generation)
            elif reason == "discard":
                if item["cookie"] != -1 or not state["flush_stack"]:
                    raise ValueError("Temporary discard lacks its native tombstone or flush")
            elif reason == "reference-release":
                previous = next(reversed(state["events"].values()), ())
                if (item["references"] != 0 or len(previous) != 4 or previous[0] != "reference-remove"
                        or previous[1] != values[0] or previous[3] != "0"):
                    raise ValueError("Temporary reference release lacks its zero-count transition")
            elif reason != "explicit-delete":
                raise ValueError("Unknown temporary removal reason")
            if state["flush_stack"] and reason in ("flush", "discard"):
                state["flushes"][state["flush_stack"][-1]]["removed"] += 1
            del state["live"][generation]
        elif kind == "scope-begin":
            if len(values) != 4:
                raise ValueError("Malformed temporary scope begin")
            epoch, cookie, allocated, depth = map(integer, values)
            if epoch != state["epoch"] or cookie < 1 or allocated not in (0, 1) or depth != len(state["scopes"]) + 1:
                raise ValueError("Temporary scope begin has another native depth or epoch")
            state["scope_events"][identity] = epoch, cookie, allocated
            state["scopes"].append(identity)
        elif kind == "scope-end":
            if len(values) != 3 or not state["scopes"]:
                raise ValueError("Temporary scope end has no open occurrence")
            scope, cookie, depth = map(integer, values)
            if scope != state["scopes"][-1] or cookie != state["scope_events"][scope][1] or depth != len(state["scopes"]) - 1:
                raise ValueError("Temporary scope end closes another occurrence")
            state["scopes"].pop()
        elif kind == "flush-begin":
            if len(values) != 2:
                raise ValueError("Malformed temporary flush begin")
            epoch, cookie = map(integer, values)
            if epoch != state["epoch"]:
                raise ValueError("Temporary flush belongs to another reset epoch")
            state["flushes"][identity] = {"epoch": epoch, "cookie": cookie, "actions": [], "removed": 0, "closed": False}
            state["flush_stack"].append(identity)
        elif kind == "flush-action":
            if len(values) != 3 or not state["flush_stack"]:
                raise ValueError("Temporary action has no native flush")
            flush, generation, created = map(integer, values)
            generation, item = selected(state, str(generation))
            if (flush != state["flush_stack"][-1] or created not in (0, 1)
                    or item["cookie"] != state["flushes"][flush]["cookie"]
                    or generation in state["pending_flush_removals"]):
                raise ValueError("Temporary flush action has another cookie or owner")
            state["flushes"][flush]["actions"].append((generation, created))
            state["pending_flush_removals"].add(generation)
        elif kind == "flush-end":
            if len(values) != 3 or not state["flush_stack"]:
                raise ValueError("Temporary flush end has no open flush")
            flush, actions, removed = map(integer, values)
            if flush != state["flush_stack"][-1]:
                raise ValueError("Temporary flush end closes another native traversal")
            selected_flush = state["flushes"][flush]
            if actions != len(selected_flush["actions"]) or removed != selected_flush["removed"]:
                raise ValueError("Temporary flush ending omits selected actions or removals")
            if any(generation in state["pending_flush_removals"] for generation, _ in selected_flush["actions"]):
                raise ValueError("Temporary flush selected an action without removing its registration")
            selected_flush["closed"] = True
            state["flush_stack"].pop()
            if selected_flush["cookie"] == 0:
                state["epoch"] += 1
        elif kind == "conditional":
            if len(values) != 3:
                raise ValueError("Malformed temporary conditional owner")
            storage = symbol(values[0], ("1",))
            true_flush, false_flush = map(integer, values[1:])
            for flush in (true_flush, false_flush):
                if flush and (flush not in state["flushes"] or not state["flushes"][flush]["closed"] or not state["flushes"][flush]["cookie"]):
                    raise ValueError("Conditional temporary cleanup has no completed branch flush")
            if true_flush and true_flush == false_flush:
                raise ValueError("Conditional branches share one cleanup traversal")
            state["conditionals"][identity] = storage, true_flush, false_flush
        else:
            raise ValueError("Unknown temporary lifetime event kind")
        state["events"][identity] = fields

    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available" and (module not in modules or not modules[module]["closed"]):
            raise ValueError("Missing temporary lifetime module closure")
        if capabilities.get(NODE_FEATURE) != "available":
            continue
        if capabilities.get(FEATURE) != "available":
            raise ValueError("Temporary cleanup nodes lack lifetime event coverage")
        state = modules[module]
        actions = {identity: fields for identity, fields in state["events"].items() if fields[0] == "flush-action"}
        if set(state["projections"]) != set(actions):
            raise ValueError("Temporary cleanup action projection coverage is incomplete")
        if state["projection_counts"] != (len(actions), len(state["nodes"])):
            raise ValueError("Temporary cleanup projection count closure is incomplete")
        observed = {}
        for node, action in state["nodes"].items():
            if node not in model.nodes or subject_modules[("node", node)] != module or action not in actions:
                raise ValueError("Temporary cleanup projection has another node or action")
            if actions[action][3] != "1":
                raise ValueError("Uncreated temporary destructor acquired a retained node")
            observed[action] = observed.get(action, 0) + 1
        if any(observed.get(action, 0) != count for action, count in state["projections"].items()):
            raise ValueError("Temporary cleanup nodes differ from their action projection")
    # EV names the actual IIf arms. Check tagged cleanup nodes against those
    # retained trees rather than trusting the true/false order of a receipt.
    branch_roots = {}
    for row in model.records["EV"]:
        parent, child = int(row[1]), int(row[2])
        if row[4] in ("true", "false") and model.properties["node", parent].get("kind") == "conditional":
            if parent not in model.nodes or child not in model.nodes:
                raise ValueError("Temporary conditional arm has a dangling retained node")
            branch_roots[child] = int(model.nodes[parent][9]), row[4]
    contexts = {}
    for identity in sorted(model.nodes):
        node = model.nodes[identity]
        contexts[identity] = branch_roots.get(identity, contexts.get(int(node[2])) if node[3] != "root" else None)
    for state in modules.values():
        roles = {}
        for storage, left, right in state["conditionals"].values():
            for flush, side in ((left, "true"), (right, "false")):
                if flush:
                    if flush in roles and roles[flush] != (storage, side):
                        raise ValueError("Temporary flush has conflicting conditional owners")
                    roles[flush] = storage, side
        for node, action in state["nodes"].items():
            flush = int(state["events"][action][1])
            if flush in roles and contexts[node] != roles[flush]:
                raise ValueError("Temporary cleanup node is retained in the wrong conditional arm")
    model.temporary_lifetimes = modules

# end of semantic_temporaries.py
