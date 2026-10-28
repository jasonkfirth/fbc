"""Project: FreeBASIC semantic sidecar
File: semantic_ir_flow.py
Purpose: Validate the actual outer-IR dispatch graph and conservative transfers.
Responsibilities: Actions, block coverage, native labels, continuations and uncertainty.
This file intentionally does NOT infer indirect destinations or runtime execution.
"""

from collections import defaultdict, deque

FEATURE = "native-ir-control-flow"
LIMIT = 1000000
PREFIXES = tuple("native-ir-" + item + ":" for item in
                 ("procedure", "action", "table", "case", "address", "block", "edge", "end", "counts"))
KINDS = frozenset(("operation", "label", "conditional", "jump", "gosub", "indirect", "switch",
                   "subroutine-return", "return-value", "assembly", "call"))
ROLES = frozenset(("binary", "unary", "convert", "store", "load", "memory", "stack", "address", "result-carrier"))
EDGES = ("fallthrough", "branch", "jump", "case", "default", "subroutine-call", "subroutine-continuation",
         "call-continuation", "assembly-continuation", "unknown-indirect", "unknown-subroutine-return",
         "unknown-assembly", "unknown-call-exit", "no-return", "procedure-exit", "return-value")


def validate_ir_flow(model, number, subject_modules):
    modules, active = {}, None
    module = 0
    loads = []
    live_types = {}
    no_return_masks = {module: int(next(row[2] for row in definitions.values() if row[0] == "status" and row[1] == "no-return"))
                       for module, definitions in model.flag_definitions.items()}

    def integer(value, minimum=0, maximum=4294967295):
        result = number(value, minimum)
        if str(result) != value or result > maximum:
            raise ValueError("Native IR scalar is not canonical or bounded")
        return result

    def symbol(value, classes, nullable=False):
        identity = integer(value)
        if not identity and nullable:
            return 0
        row = model.symbols.get(identity)
        if row is None or row[3] not in classes or subject_modules.get(("symbol", identity)) != module:
            raise ValueError("Native IR symbol has another class or module")
        return identity

    for row in model.rows:
        if row[0] == "M":
            if active is not None or loads:
                raise ValueError("Native IR graph or load spans modules")
            module += 1
            continue
        if row[0] == "T":
            live_types[int(row[1])] = row
        if row[0] == "K" and row[3].startswith("native-load-enter:"):
            loads.append(int(row[3].split(":")[1]))
        elif row[0] == "K" and row[3].startswith("native-load-end:"):
            if not loads or loads.pop() != int(row[3].split(":")[1]):
                raise ValueError("Native IR load context is unmatched")
        if row[0] != "K" or not row[3].startswith(PREFIXES):
            continue
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Native IR graph has unavailable coverage")
        owner = symbol(row[2], {"8"})
        if row[1] != "symbol" or model.symbols[owner][11] != "0":
            raise ValueError("Native IR observation lacks its module namespace")
        state = modules.setdefault(module, {"graphs": {}, "closure": None})
        if state["closure"] is not None:
            raise ValueError("Native IR observation follows its module closure")
        parts = row[3].split(":")
        family, graph_id = parts[0][len("native-ir-"):], integer(parts[1], 1, LIMIT)
        fields = row[4].split("\t")
        if family == "counts":
            if len(parts) != 2 or graph_id != module or active is not None or len(fields) != 6:
                raise ValueError("Malformed native IR module closure")
            state["closure"] = [integer(value, 0, LIMIT) for value in fields]
            continue
        if family == "procedure":
            if len(parts) != 2 or active is not None or graph_id != len(state["graphs"]) + 1 or len(fields) != 4:
                raise ValueError("Native IR procedure order or ownership is invalid")
            symbol(fields[0], {"3"})
            symbol(fields[1], {"7"})
            if fields[2] not in ("gcc", "clang", "llvm", "gas", "gas64") or fields[3] != "outer-ir-dispatch":
                raise ValueError("Unknown native IR backend or abstraction")
            graph = {"header": fields, "actions": {}, "tables": {}, "cases": defaultdict(dict),
                     "addresses": {}, "blocks": {}, "edges": {}, "end": None}
            state["graphs"][graph_id] = graph
            active = module, graph_id
            continue
        if active != (module, graph_id):
            raise ValueError("Native IR observation lacks its active procedure")
        graph = state["graphs"][graph_id]
        if family == "end":
            if len(parts) != 2 or len(fields) != 7:
                raise ValueError("Malformed native IR procedure closure")
            end = [integer(value, 0, LIMIT if i < 5 else 4294967295) for i, value in enumerate(fields)]
            symbol(fields[5], {"7"})
            if end[6] not in (0, 1):
                raise ValueError("Native IR unknown-transfer flag is invalid")
            graph["end"] = end
            active = None
            continue
        if len(parts) < 3:
            raise ValueError("Native IR observation lacks its ordinal")
        identity = integer(parts[2], 0 if family == "case" else 1, LIMIT)
        if family in ("action", "address", "block", "edge"):
            bucket = graph[family + ("es" if family == "address" else "s")]
            if len(parts) != 3 or identity != len(bucket) + 1:
                raise ValueError("Native IR observations are not dense or repeat")
            bucket[identity] = fields
        if family == "action":
            if len(fields) != 8 or fields[0] not in KINDS:
                raise ValueError("Malformed native IR action")
            kind = fields[0]
            integer(fields[1], 1, LIMIT)
            load = integer(fields[2], 0, LIMIT)
            if load != (loads[-1] if loads else 0):
                raise ValueError("Native IR action has another actual load context")
            if load:
                frame = model.native_loads[module]["enters"][load]
                if frame[1] != graph["header"][0] or frame[8] != "1":
                    raise ValueError("Native IR action belongs to another or unemitted body")
            integer(fields[3], -1)
            target = symbol(fields[4], {"7"}, True)
            callee = symbol(fields[5], {"3"}, True)
            integer(fields[7], 0)
            if kind in ("label", "conditional", "jump", "gosub", "switch") and not target:
                raise ValueError("Native IR transfer loses its actual label")
            if kind not in ("label", "conditional", "jump", "gosub", "switch") and target:
                raise ValueError("Native IR action invents a direct target")
            if kind == "call":
                if not callee or fields[3] not in ("0", "1") or fields[6] not in ("may-return", "known-no-return", "runtime-end"):
                    raise ValueError("Invalid native IR call target or return policy")
                declaration = live_types.get(callee)
                if declaration is None or module not in no_return_masks:
                    raise ValueError("Native IR call lacks its observed declaration flags")
                expected_policy = "may-return"
                if int(declaration[12]) & no_return_masks[module]:
                    expected_policy = "known-no-return"
                elif fields[3] == "0" and declaration[18] == "runtime" and declaration[15] == "fb_End":
                    expected_policy = "runtime-end"
                if fields[6] != expected_policy:
                    raise ValueError("Native IR call invents or loses a known return policy")
            elif callee:
                raise ValueError("Non-call IR action invents a procedure target")
            if kind == "operation" and fields[6] not in ROLES:
                raise ValueError("Unknown native IR operation role")
            if kind == "label" and fields[6] not in ("normal", "no-flush"):
                raise ValueError("Unknown native IR label kind")
            if kind == "assembly" and fields[6] != "unknown-memory-registers-control":
                raise ValueError("Assembly IR action claims known effects")
        elif family == "address":
            if len(fields) != 2:
                raise ValueError("Malformed native IR label-address receipt")
            symbol(fields[0], {"7"})
            if integer(fields[1], 0, LIMIT) != (loads[-1] if loads else 0):
                raise ValueError("Native IR label address has another load context")
        elif family == "table":
            if len(parts) != 3 or identity not in graph["actions"] or graph["actions"][identity][0] != "switch" or identity in graph["tables"] or len(fields) != 2:
                raise ValueError("Native IR jump table lacks its actual dispatch")
            graph["tables"][identity] = [integer(value, 0, 18446744073709551615) for value in fields]
        elif family == "case":
            if len(parts) != 4 or len(fields) != 2 or identity not in graph["tables"]:
                raise ValueError("Native IR jump case lacks its table")
            ordinal = integer(parts[3], 0, LIMIT)
            if ordinal != len(graph["cases"][identity]):
                raise ValueError("Native IR jump cases are not dense")
            integer(fields[0], 0, 18446744073709551615)
            symbol(fields[1], {"7"})
            graph["cases"][identity][ordinal] = fields
        elif family == "block":
            if len(fields) != 3 or fields[2] not in ("entry-reachable", "unknown-entry", "unreachable"):
                raise ValueError("Unknown native IR block reachability")
            integer(fields[0], 0, LIMIT)
            integer(fields[1], 0, LIMIT)
        elif family == "edge":
            if len(fields) != 5 or fields[2] not in EDGES:
                raise ValueError("Malformed native IR edge")
            integer(fields[0], 1, LIMIT)
            integer(fields[1], 0, LIMIT)
            symbol(fields[3], {"7"}, True)
            guard = integer(fields[4], 0, len(EDGES))
            if guard and (fields[2] != "procedure-exit" or guard not in (1, 7, 8, 9)):
                raise ValueError("Native IR exit has another continuation guard")
        else:
            raise ValueError("Unknown native IR observation family")
    if active is not None or loads:
        raise ValueError("Native IR procedure or load remains open")

    model.native_ir_flow = modules
    for module in range(1, len(model.records["M"]) + 1):
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            continue
        state = modules.get(module)
        if state is None or state["closure"] is None:
            raise ValueError("Missing native IR module coverage")
        totals = [len(state["graphs"]), 0, 0, 0, 0, 0]
        procedures = set()
        for graph in state["graphs"].values():
            actions, blocks, edges, end = graph["actions"], graph["blocks"], graph["edges"], graph["end"]
            counts = [len(actions), len(blocks), len(edges), sum(map(len, graph["cases"].values())), len(graph["addresses"])]
            if end is None or end[:5] != counts or not blocks:
                raise ValueError("Native IR procedure coverage is incomplete")
            for index, count in enumerate(counts, 1):
                totals[index] += count
            procedure = int(graph["header"][0])
            if procedure in procedures:
                raise ValueError("Native IR procedure was emitted twice")
            procedures.add(procedure)
            labels = {}
            for action in actions.values():
                if action[0] == "label":
                    target = int(action[4])
                    if target in labels:
                        raise ValueError("Repeated native IR label definition")
                    labels[target] = int(action[1])
            expected_edges = []
            next_action = 1
            unknown = False
            for block_id, block in blocks.items():
                first, last = map(int, block[:2])
                if not actions:
                    if len(blocks) != 1 or first or last:
                        raise ValueError("Empty native IR procedure invents actions")
                    expected_edges.append((block_id, 0, "procedure-exit", 0, 0))
                    continue
                if first != next_action or last < first or last > len(actions):
                    raise ValueError("Native IR action ranges overlap or have gaps")
                next_action = last + 1
                members = [actions[i] for i in range(first, last + 1)]
                if any(int(item[1]) != block_id for item in members):
                    raise ValueError("Native IR action claims another block")
                if any(item[0] not in ("operation", "label") for item in members[:-1]) or any(item[0] == "label" for item in members[1:]):
                    raise ValueError("Native IR basic block crosses a transfer or label")
                tail = members[-1]
                kind, target, policy = tail[0], int(tail[4]), tail[6]
                continuation = "fallthrough"
                def direct(label, edge_kind):
                    if label not in labels:
                        raise ValueError("Native IR transfer lacks its actual label definition")
                    expected_edges.append((block_id, labels[label], edge_kind, label, 0))
                if kind == "conditional":
                    direct(target, "branch")
                elif kind == "jump":
                    direct(target, "jump");continuation = None
                elif kind == "gosub":
                    direct(target, "subroutine-call");continuation = "subroutine-continuation"
                elif kind == "switch":
                    if last not in graph["tables"] or len(graph["cases"][last]) != int(tail[7]):
                        raise ValueError("Native IR jump table loses actual cases")
                    direct(target, "default")
                    for case in graph["cases"][last].values():
                        direct(int(case[1]), "case")
                    continuation = None
                elif kind in ("indirect", "subroutine-return", "return-value"):
                    edge_kind = {"indirect": "unknown-indirect", "subroutine-return": "unknown-subroutine-return", "return-value": "return-value"}[kind]
                    expected_edges.append((block_id, 0, edge_kind, 0, 0));continuation = None
                    unknown |= kind != "return-value"
                elif kind == "assembly":
                    expected_edges.append((block_id, 0, "unknown-assembly", 0, 0));continuation = "assembly-continuation";unknown = True
                elif kind == "call":
                    edge_kind = "unknown-call-exit" if policy == "may-return" else "no-return"
                    expected_edges.append((block_id, 0, edge_kind, 0, 0))
                    continuation = "call-continuation" if policy == "may-return" else None
                    unknown |= policy != "runtime-end"
                if continuation:
                    if block_id < len(blocks):
                        expected_edges.append((block_id, block_id + 1, continuation, 0, 0))
                    else:
                        guard = 0 if kind in ("operation", "label") else {"fallthrough": 1, "subroutine-continuation": 7, "call-continuation": 8, "assembly-continuation": 9}[continuation]
                        expected_edges.append((block_id, 0, "procedure-exit", 0, guard))
            if next_action != len(actions) + 1:
                raise ValueError("Native IR block membership is incomplete")
            actual_edges = [(int(row[0]), int(row[1]), row[2], int(row[3]), int(row[4])) for row in edges.values()]
            # Case labels can repeat for distinct values. Compare a multiset,
            # preserving that native coverage while rejecting repaired totals.
            from collections import Counter
            if Counter(actual_edges) != Counter(expected_edges) or bool(end[6]) != bool(unknown):
                raise ValueError("Native IR edges invent or lose actual transfers/continuations")
            reachable, queue = {1}, deque((1,))
            successors = defaultdict(list)
            for source, target, *_ in actual_edges:
                if target:
                    successors[source].append(target)
            while queue:
                for target in successors[queue.popleft()]:
                    if target not in reachable:
                        reachable.add(target);queue.append(target)
            for identity, block in blocks.items():
                expected = "entry-reachable" if identity in reachable else "unknown-entry" if unknown else "unreachable"
                if block[2] != expected:
                    raise ValueError("Native IR block claims unproved reachability")
        emitted = {int(row[2]) for row in model.records["PH"] if row[4] == "1" and subject_modules.get(("symbol", int(row[2]))) == module}
        if totals != state["closure"] or procedures != emitted:
            raise ValueError("Native IR module loses an actually emitted procedure")


def validate_root_transfer_policies(model, number, subject_modules):
    feature = "preload-root-transfer-policies"
    phases = {int(row[1]): int(row[2]) for row in model.records["NP"]}
    blocks = {int(row[2]): int(row[1]) for row in model.records["CN"]}
    outgoing = defaultdict(list)
    for edge in model.records["CE"]:
        outgoing[int(edge[2])].append(edge)
    left_children = {int(row[2]) for row in model.nodes.values() if row[3] == "left"}
    masks = {module: int(next(row[2] for row in definitions.values() if row[0] == "status" and row[1] == "no-return"))
             for module, definitions in model.flag_definitions.items()}
    for identity, node in model.nodes.items():
        source_module = subject_modules["node", identity]
        available = model.capabilities.get(source_module, {}).get(feature) == "available"
        actual = model.properties["node", identity].get("root-flow-stop")
        if not available:
            if actual is not None:
                raise ValueError("Unavailable preload transfer coverage has a stop policy")
            continue
        expected = None
        if node[3] == "root" and identity in phases:
            if node[4] == "1" and model.properties["node", identity].get("result-load") == "1":
                expected = "return-value"
            elif node[4] == "9":
                selected = int(node[9])
                declaration = model.types[selected]
                if int(declaration[12]) & masks[source_module]:
                    expected = "known-no-return"
                elif identity not in left_children and declaration[18] == "runtime" and declaration[15] == "fb_End":
                    expected = "runtime-end"
        if actual != expected:
            raise ValueError("Preload root transfer policy is incomplete or contradicts its native action")
        if expected is not None:
            edges = outgoing[blocks[identity]]
            if len(edges) != 1 or edges[0][3:] != ["0", "procedure-exit", "0"]:
                raise ValueError("Terminating preload root invents a fallthrough edge")

# end of semantic_ir_flow.py
