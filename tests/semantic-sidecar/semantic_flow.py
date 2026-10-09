"""Project: FreeBASIC semantic sidecar tests
File: semantic_flow.py
Purpose: Validate the compiler's recorded procedure phases and transfers.
Responsibilities: Check ownership, block membership, ordering and graph closure.
This file intentionally does NOT infer branches or parse FreeBASIC programs.
"""

from collections import defaultdict


def validate_flow(model, number, storage_roots=()) -> None:
    """Use the caller's checked wire-number decoder; missing edges stay missing."""
    phases = {}
    for row in model.records["PH"]:
        identity = number(row[1], 1)
        if identity in phases or row[3] != "pre-load" or row[4] not in ("0", "1"):
            raise ValueError("Invalid or repeated procedure phase")
        if number(row[2], 1) not in model.signatures:
            raise ValueError("Procedure phase lacks its signature")
        phases[identity] = row

    node_phases = {}
    for row in model.records["NP"]:
        identity, phase = number(row[1], 1), number(row[2], 1)
        if identity not in model.nodes or phase not in phases or identity in node_phases:
            raise ValueError("Invalid or repeated node phase")
        node_phases[identity] = phase

    # Parser receiver snapshots have already passed independent storage and
    # ownership checks. They describe addresses before lowering, not execution.
    initializer_roots = {number(row[4], 1) for row in model.records["H"]
                         if row[1] == "symbol" and row[3] == "node"
                         and row[5] in ("initializer", "default-initializer")}
    initializer_roots.update(storage_roots)
    roots = {}
    for identity in sorted(model.nodes):
        row = model.nodes[identity]
        parent = number(row[2], 1)
        root = identity if row[3] == "root" else roots[parent]
        roots[identity] = root
        phase = node_phases.get(identity)
        if phase is None:
            if root not in initializer_roots:
                raise ValueError("AST phase membership is incomplete; tree lacks its observed phase or initializer owner")
        elif row[3] == "root":
            if phases[phase][2] != row[2]:
                raise ValueError("Phase root belongs to another procedure")
        elif node_phases.get(parent) != phase:
            raise ValueError("AST child belongs to another phase")

    conditions = {"always", "argument", "profile-begin", "call-target", "profile-end",
                  "copyback", "condition", "true", "false", "result"}
    evaluation_edges = set()
    for row in model.records["EV"]:
        parent, child = number(row[1], 1), number(row[2], 1)
        number(row[3], 0)
        if parent == child or parent not in node_phases or child not in node_phases:
            raise ValueError("Evaluation edge lacks distinct phased nodes")
        if node_phases[parent] != node_phases[child] or row[4] not in conditions:
            raise ValueError("Evaluation condition or phase is invalid")
        edge = parent, child
        if edge in evaluation_edges:
            raise ValueError("Repeated evaluation edge")
        evaluation_edges.add(edge)

    blocks = {}
    ordinals = defaultdict(set)
    for row in model.records["CB"]:
        identity, phase, ordinal = [number(value, 1 if index < 2 else 0)
                                    for index, value in enumerate(row[1:])]
        if identity in blocks or phase not in phases or ordinal in ordinals[phase]:
            raise ValueError("Invalid or repeated control-flow block")
        blocks[identity] = row
        ordinals[phase].add(ordinal)
    if any(values != set(range(len(values))) for values in ordinals.values()):
        raise ValueError("Control-flow block ordinals have gaps")

    block_nodes = {}
    phased_roots = {identity for identity in node_phases if model.nodes[identity][3] == "root"}
    for row in model.records["CN"]:
        block, identity = number(row[1], 1), number(row[2], 1)
        if block not in blocks or identity not in phased_roots or row[3] != "0":
            raise ValueError("Control-flow member is not a phased root")
        if block in block_nodes or node_phases[identity] != int(blocks[block][2]):
            raise ValueError("Repeated block member or wrong phase")
        block_nodes[block] = identity
    if set(block_nodes) != set(blocks) or set(block_nodes.values()) != phased_roots:
        raise ValueError("Control-flow root/block membership is incomplete")
    if len(block_nodes) != len(phased_roots):
        raise ValueError("AST root belongs to more than one control-flow block")

    labels = set()
    for row in model.records["CL"]:
        phase, label, block = [number(value, 1) for value in row[1:]]
        if block not in blocks or phase != int(blocks[block][2]) or (phase, label) in labels:
            raise ValueError("Control-flow label has a repeated or foreign block")
        node = model.nodes[block_nodes[block]]
        if (node[4] != "21" or number(node[9], 1) != label
                or label not in model.types or model.types[label][3] != "label"):
            raise ValueError("Control-flow label differs from its recorded AST label")
        labels.add((phase, label))

    native_targets = defaultdict(set)
    default_targets = defaultdict(set)
    case_targets = defaultdict(set)
    for row in model.records["H"]:
        if row[1] == "node" and row[3] == "symbol":
            if row[5] == "branch-target":
                native_targets[int(row[2])].add(int(row[4]))
            elif row[5] == "default-target":
                default_targets[int(row[2])].add(int(row[4]))
    for row in model.records["J"]:
        case_targets[int(row[1])].add(int(row[4]))

    label_kinds = {"label", "conditional-label", "case-label", "default-label", "subroutine-call"}
    unknown_kinds = {"unknown-indirect", "subroutine-return", "unknown-assembly", "procedure-exit"}
    for row in model.records["CE"]:
        phase, source, target = number(row[1], 1), number(row[2], 1), number(row[3], 0)
        label = number(row[5], 0)
        if source not in blocks or phase != int(blocks[source][2]) or (label and label not in model.symbols):
            raise ValueError("Control-flow edge has a foreign source or missing label")
        kind = row[4]
        if kind == "fallthrough":
            if target not in blocks or phase != int(blocks[target][2]) or label:
                raise ValueError("Fallthrough target differs from its phase")
            if int(blocks[target][3]) != int(blocks[source][3]) + 1:
                raise ValueError("Fallthrough is not the next recorded block")
        elif kind in label_kinds:
            if target or (not label and kind != "subroutine-call"):
                raise ValueError("Label transfer has an incompatible target")
            if label:
                # A symbol identity alone is insufficient: the transfer must
                # reach a recorded label in this phase and retain the target
                # actually selected by the native AST.
                if (phase, label) not in labels:
                    raise ValueError("Control-flow target lacks a label in its phase")
                node = block_nodes[source]
                if kind == "case-label":
                    targets = case_targets[node]
                elif kind == "default-label":
                    targets = default_targets[node]
                else:
                    selected = model.properties["node", node].get("sequence-tail-branch-node")
                    if selected is not None:
                        node = number(selected, 1)
                    targets = native_targets[node]
                if label not in targets:
                    raise ValueError("Control-flow target differs from its recorded native target")
        elif kind in unknown_kinds:
            if target or label:
                raise ValueError("Unresolved transfer invents a target")
        else:
            raise ValueError("Unknown control-flow transfer kind")


def validate_sequence_branches(model, number, subject_modules):
    """A final LINK right-child operator supplies one actual conditional CE."""
    right_nodes = {int(row[2]): identity for identity, row in model.nodes.items() if row[3] == "right"}
    targets = {}
    for row in model.relations("branch-target"):
        if row[1] == "node" and row[3] == "symbol":
            node = int(row[2])
            if node in targets:
                raise ValueError("Repeated native branch target")
            targets[node] = int(row[4])
    evaluations = {(int(row[1]), int(row[2])): row[4] for row in model.records["EV"]}
    phases = {int(row[1]): row[2] for row in model.records["NP"]}
    labels = {(row[1], int(row[2])) for row in model.records["CL"]}
    block_nodes = {int(row[1]): int(row[2]) for row in model.records["CN"]}
    branch_edges = {}
    for row in model.records["CE"]:
        node = block_nodes[int(row[2])]
        if model.nodes[node][4] == "15" and row[4] == "conditional-label":
            if node in branch_edges:
                raise ValueError("Repeated sequence conditional edge")
            branch_edges[node] = row
    work_left = 4000000 - len(model.nodes) - len(model.records["H"]) - len(model.records["CE"])
    def tail(node):
        nonlocal work_left
        if model.nodes[node][4] != "15":
            return None
        for _ in range(65537):
            work_left -= 1
            if work_left < 0:
                raise ValueError("Sequence tail work budget exhausted")
            if model.nodes[node][4] != "15":
                return node if model.nodes[node][4] in ("3", "4") else None
            child = right_nodes.get(node)
            if child is None:
                return None
            if evaluations.get((node, child)) != "always":
                raise ValueError("Sequence right-child evaluation receipt is incomplete")
            node = child
        raise ValueError("Sequence tail depth budget exhausted")
    for (domain, identity), properties in model.properties.items():
        if any(key.startswith("sequence-tail-branch-") and key != "sequence-tail-branch-node" for key in properties):
            raise ValueError("Unknown sequence tail branch property")
        if "sequence-tail-branch-node" not in properties:
            continue
        work_left -= 1
        module = subject_modules.get((domain, identity))
        if (domain != "node" or identity not in model.nodes or model.nodes[identity][3] != "root"
                or model.capabilities[module].get("sequence-tail-branch-contracts") != "available"):
            raise ValueError("Sequence branch property lacks native capability and root")
        selected = number(properties["sequence-tail-branch-node"], 1)
        actual = tail(identity)
        edge = branch_edges.get(identity)
        label = targets.get(actual)
        phase = phases.get(identity)
        if (selected != actual or label is None or phases.get(actual) != phase
                or subject_modules.get(("node", actual)) != module or (phase, label) not in labels
                or edge is None or edge[1] != phase or int(edge[5]) != label):
            raise ValueError("Sequence tail differs from its branch, phase or edge")
    for identity, row in model.nodes.items():
        work_left -= 1
        module = subject_modules["node", identity]
        if identity in branch_edges and "sequence-tail-branch-node" not in model.properties["node", identity]:
            raise ValueError("Sequence conditional edge lacks its selected tail")
        if (row[3] == "root" and row[4] == "15" and identity in phases
                and model.capabilities[module].get("sequence-tail-branch-contracts") == "available"):
            actual = tail(identity)
            if actual in targets and (phases[identity], targets[actual]) in labels and "sequence-tail-branch-node" not in model.properties["node", identity]:
                raise ValueError("Sequence conditional tail lacks its mandatory property")
            if identity in branch_edges and actual not in targets:
                raise ValueError("Sequence edge has no actual tail transfer")
    if work_left < 0:
        raise ValueError("Sequence tail work budget exhausted")


# end of semantic_flow.py
