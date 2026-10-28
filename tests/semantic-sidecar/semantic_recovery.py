"""Project: FreeBASIC semantic sidecar
File: semantic_recovery.py
Purpose: Validate native parser recovery roots and their phase ownership.
Responsibilities: Dense event counts, raw root snapshots and source context closure.
This file intentionally does NOT identify a later AST node from a recycled pointer.
"""

FEATURE = "native-expression-recovery-steps"
PREFIX = "native-expression-recovery"
KINDS = {"replacement-constant", "replacement-zero", "retained-prefix", "replacement-expression", "unformattable-type"}
FILES = {
    'tooling/semantic-model.bas',
    'parser/declarations/parser-decl-const.bas',
    'parser/declarations/parser-decl-proc-params.bas',
    'parser/declarations/parser-decl-struct.bas',
    'parser/declarations/parser-decl-symb-init.bas',
    'parser/declarations/parser-decl-symbtype.bas',
    'parser/declarations/parser-decl-var.bas',
    'parser/expressions/parser-expr-atom.bas',
    'parser/expressions/parser-expr-binary.bas',
    'parser/expressions/parser-expr-function.bas',
    'parser/expressions/parser-expr-unary.bas',
    'parser/expressions/parser-expr-variable.bas',
    'parser/intrinsics/parser-quirk-array.bas',
    'parser/intrinsics/parser-quirk-casting.bas',
    'parser/intrinsics/parser-quirk-console.bas',
    'parser/intrinsics/parser-quirk-file.bas',
    'parser/intrinsics/parser-quirk-gfx.bas',
    'parser/intrinsics/parser-quirk-iif.bas',
    'parser/intrinsics/parser-quirk-math.bas',
    'parser/intrinsics/parser-quirk-mem.bas',
    'parser/intrinsics/parser-quirk-on.bas',
    'parser/intrinsics/parser-quirk-peekpoke.bas',
    'parser/intrinsics/parser-quirk-string.bas',
    'parser/intrinsics/parser-quirk-vafirst.bas',
    'parser/parser-helper.bas',
    'parser/procedures/parser-proccall-args.bas',
    'parser/procedures/parser-proccall.bas',
    'parser/statements/parser-compound-do.bas',
    'parser/statements/parser-compound-for.bas',
    'parser/statements/parser-compound-if.bas',
    'parser/statements/parser-compound-select-const.bas',
    'parser/statements/parser-compound-select.bas',
    'parser/statements/parser-compound-while.bas',
}


def validate_recovery(model, number, unescape, node_kinds):
    steps, counts = {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = number(row[2], 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) not in ("partial", "available"):
            raise ValueError("Unavailable or unowned native recovery observation")
        if row[3] == PREFIX + "-count":
            if module in counts:
                raise ValueError("Repeated native recovery count")
            counts[module] = number(row[4], 0)
            if counts[module] > 100000 or str(counts[module]) != row[4]:
                raise ValueError("Noncanonical or oversized native recovery count")
            continue
        parts = row[3].split(":")
        ordinal = number(parts[1], 1) if len(parts) == 2 else 0
        observed = steps.setdefault(module, [])
        if parts[0] != PREFIX or len(parts) != 2 or str(ordinal) != parts[1] or ordinal != len(observed) + 1 or ordinal > 100000:
            raise ValueError("Malformed or unordered native recovery identity")
        fields = row[4].split("\t")
        if len(fields) != 17 or fields[0] not in KINDS:
            raise ValueError("Malformed native recovery root")
        fields[1], fields[2], fields[11] = (unescape(fields[index]) for index in (1, 2, 11))
        if not fields[1] or fields[2] not in FILES or any("\0" in fields[index] for index in (1, 2, 11)):
            raise ValueError("Invalid native recovery parser route")
        for index in (3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16):
            value = number(fields[index], 0)
            maximum = 4294967295 if index == 10 else 2147483647
            if value > maximum or str(value) != fields[index]:
                raise ValueError("Noncanonical or oversized native recovery scalar")
        if fields[3] == "0" or fields[8] not in ("0", "1") or fields[16] not in ("0", "1"):
            raise ValueError("Invalid native recovery site or flags")
        statement, source, configuration = map(int, fields[4:7])
        if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
            raise ValueError("Native recovery step has a dangling statement owner")
        if source not in model.source_contexts or int(model.source_contexts[source][4]) != module:
            raise ValueError("Native recovery step has a dangling source context")
        if ((configuration and model.configurations.get(configuration) != module)
                or (not configuration and not model.expressions_only)):
            raise ValueError("Native recovery step has a dangling configuration")
        root_class = int(fields[9])
        unformattable = fields[0] == "unformattable-type"
        if unformattable != (fields[2] == "tooling/semantic-model.bas") or (unformattable and fields[8] != "1"):
            raise ValueError("Unformattable type observation has no root or native export site")
        if fields[8] == "0":
            if root_class != 0 or fields[10] != "2147483648":
                raise ValueError("Absent recovery root invents an AST payload")
        elif root_class >= len(node_kinds):
            raise ValueError("Unknown native recovery root class")
        elif fields[0] == "replacement-constant" and node_kinds[root_class] != "constant":
            raise ValueError("Replacement constant has another active AST class")
        if fields[16] == "1":
            if not fields[11] or int(fields[14]) < 1:
                raise ValueError("Physical recovery point has no source line")
        elif fields[14:16] != ["0", "0"]:
            raise ValueError("Unavailable recovery point invents physical coordinates")
        observed.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) in ("partial", "available") and counts.get(module) != len(steps.get(module, ())):
            raise ValueError("Native recovery observation coverage is incomplete")
    model.recovery_steps = steps
    affected = {int(fields[4]) for group in steps.values() for fields in group if int(fields[4])}
    for (domain, identity), statement in model.statement_owners.items():
        if domain == "expression" and statement in affected:
            model.expression_validity[identity] = "recovery-affected"

# end of semantic_recovery.py
