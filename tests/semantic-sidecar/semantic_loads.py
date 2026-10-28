"""Project: FreeBASIC semantic sidecar
File: semantic_loads.py
Purpose: Validate actual recursive AST-load order and late semantic actions.
Responsibilities: Origins, interval closure, emitted calls and check-handler uncertainty.
This file intentionally does NOT treat compiler loading order as runtime execution.
"""

FEATURE = "native-ast-load-actions"
PREFIXES = ("native-load-enter:", "native-load-end:", "native-load-call:", "native-load-check:", "native-load-counts:")
LIMIT = 1000000


def validate_loads(model, number, subject_modules, node_kinds):
    modules = {}
    module = 0

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Native load scalar is not canonical or bounded")
        return result

    def symbol(value, classes):
        identity = integer(value, 1)
        native = model.symbols.get(identity)
        if (native is None or native[3] not in classes
                or subject_modules.get(("symbol", identity)) != module):
            raise ValueError("Native load symbol has another class or module")
        return identity

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Native load observation lacks its capability")
        owner = symbol(row[2], ("8",))
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Native load observation lacks its module namespace")
        state = modules.setdefault(module, {"enters": {}, "ends": {}, "calls": {}, "checks": {}, "stack": [], "closed": False})
        if state["closed"]:
            raise ValueError("Native load observation follows module closure")
        parts = row[3].split(":")
        if len(parts) != 2:
            raise ValueError("Malformed native load key")
        family, identity = parts[0], integer(parts[1], 1, LIMIT)
        fields = tuple(row[4].split("\t"))
        if family == "native-load-counts":
            if identity != module or len(fields) != 4:
                raise ValueError("Malformed native load count closure")
            actual = tuple(integer(value, 0, LIMIT) for value in fields)
            expected = tuple(len(state[key]) for key in ("enters", "ends", "calls", "checks"))
            if actual != expected or state["stack"] or set(state["enters"]) != set(state["ends"]):
                raise ValueError("Native load intervals or count closure are incomplete")
            state["closed"] = True
        elif family == "native-load-enter":
            if identity != len(state["enters"]) + 1 or len(fields) != 10:
                raise ValueError("Native load entry order is incomplete")
            parent, proc, origin, inherited, raw_class, statement, source, context, emitted, depth = map(integer, fields)
            symbol(fields[1], ("3",))
            if raw_class >= len(node_kinds) or emitted not in (0, 1) or depth != len(state["stack"]) + 1:
                raise ValueError("Native load entry has another class, flag or depth")
            expected_parent = state["stack"][-1] if state["stack"] else 0
            if parent != expected_parent:
                raise ValueError("Native load entry has another active parent")
            expected_origin = 0
            if parent:
                ancestor = state["enters"][parent]
                expected_origin = int(ancestor[2]) or int(ancestor[3])
                if ancestor[1] != fields[1]:
                    raise ValueError("Nested native load belongs to another procedure")
            if inherited != expected_origin:
                raise ValueError("Native load inherited another snapshot origin")
            for native in (origin, inherited):
                if native and (native not in model.nodes or subject_modules[("node", native)] != module):
                    raise ValueError("Native load origin is dangling or foreign")
            if statement and (statement not in model.statements or model.statements[statement][7] != str(module)):
                raise ValueError("Native load entry has a foreign statement")
            if source and (source not in model.source_contexts or model.source_contexts[source][4] != str(module)):
                raise ValueError("Native load entry has a foreign source occurrence")
            if context and model.configurations.get(context) != module:
                raise ValueError("Native load entry has a foreign configuration")
            state["enters"][identity] = fields
            state["stack"].append(identity)
        elif family == "native-load-end":
            if len(fields) != 1 or fields[0] not in ("0", "1") or not state["stack"] or identity != state["stack"][-1]:
                raise ValueError("Native load return closes another entry")
            if identity in state["ends"]:
                raise ValueError("Repeated native load return")
            if state["enters"][identity][8] == "0" and fields[0] != "0":
                raise ValueError("Unemitted native load acquired an observed IR result")
            state["ends"][identity] = fields[0]
            state["stack"].pop()
        else:
            key = "calls" if family == "native-load-call" else "checks"
            if identity != len(state[key]) + 1 or len(fields) != 5:
                raise ValueError("Native load action order or payload is incomplete")
            load_id = integer(fields[0], 1, LIMIT)
            if not state["stack"] or load_id != state["stack"][-1]:
                raise ValueError("Native emitted action has no active load")
            entry = state["enters"][load_id]
            if entry[8] != "1":
                raise ValueError("Unemitted native load acquired an emitted action")
            kind = node_kinds[int(entry[4])]
            if key == "calls":
                symbol(fields[1], ("3",))
                if kind != "call" or fields[2] not in ("0", "1") or fields[3] not in ("call", "profile-end") or fields[4] not in ("0", "1"):
                    raise ValueError("Native call action has another load class, role or flag")
                if fields[3] == "profile-end" and fields[2] != "0":
                    raise ValueError("Native profiling completion acquired an indirect target")
            else:
                symbol(fields[2], ("7",))
                if (kind, fields[1]) not in (("bounds-check", "bounds-or-dimensions"), ("pointer-check", "null-pointer")):
                    raise ValueError("Native check transfer has another loader class")
                if fields[3:] != ("nonzero-handler", "unknown-indirect"):
                    raise ValueError("Native check transfer lost its condition or target uncertainty")
            state[key][identity] = fields
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available" and (module not in modules or not modules[module]["closed"]):
            raise ValueError("Missing native load module closure")
    model.native_loads = modules

# end of semantic_loads.py
