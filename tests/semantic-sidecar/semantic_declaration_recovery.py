"""Project: FreeBASIC semantic sidecar
File: semantic_declaration_recovery.py
Purpose: Validate explicit recovery type choices and transient declaration snapshots.
Responsibilities: Canonical payloads, context ownership and exact event closure.
This file intentionally does NOT turn recovery names into resolved source bindings.
"""

TYPE_FEATURE = "native-type-recovery-steps"
SYMBOL_FEATURE = "native-symbol-recovery-steps"
TYPE_PREFIX = "native-type-recovery"
SYMBOL_PREFIX = "native-symbol-recovery"
FILES = {
    "parser/parser-helper.bas", "parser/expressions/parser-expr-unary.bas",
    "parser/intrinsics/parser-quirk-peekpoke.bas",
    "parser/declarations/parser-decl-const.bas", "parser/declarations/parser-decl-struct.bas",
    "parser/declarations/parser-decl-var.bas", "parser/declarations/parser-decl-proc-params.bas",
    "parser/declarations/parser-decl-symbtype.bas", "parser/declarations/parser-decl-typedef.bas",
    "parser/declarations/parser-decl-enum.bas",
}


def scalar(text, number, minimum=0, maximum=2147483647):
    value = number(text, minimum)
    if value > maximum or str(value) != text:
        raise ValueError("Noncanonical or oversized declaration recovery scalar")
    return value


def site(fields, model, module, number, unescape):
    if len(fields) != 13:
        raise ValueError("Malformed declaration recovery native site")
    fields = fields.copy()
    for index in (0, 1, 6):
        fields[index] = unescape(fields[index])
        if "\0" in fields[index]:
            raise ValueError("Null byte in declaration recovery site")
    if not fields[0] or fields[1] not in FILES:
        raise ValueError("Unknown declaration recovery parser route")
    for index in (2, 3, 4, 5, 7, 8, 9, 10, 11, 12):
        scalar(fields[index], number)
    if fields[2] == "0" or fields[12] not in ("0", "1"):
        raise ValueError("Invalid declaration recovery site or physical flag")
    statement, configuration, source = map(int, (fields[3], fields[4], fields[7]))
    if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
        raise ValueError("Declaration recovery has a dangling statement")
    if ((configuration and model.configurations.get(configuration) != module)
            or (not configuration and not model.expressions_only)):
        raise ValueError("Declaration recovery has a dangling configuration")
    if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
        raise ValueError("Declaration recovery has a dangling source context")
    if fields[12] == "1":
        if not source or not fields[6] or int(fields[10]) < 1:
            raise ValueError("Physical declaration recovery point has no source")
    elif fields[10:12] != ["0", "0"]:
        raise ValueError("Declaration recovery invents physical coordinates")
    return fields


def validate_declaration_recovery(model, number, unescape):
    types, symbols, counts = {}, {}, {}
    for row in model.records["K"]:
        prefix = TYPE_PREFIX if row[3].startswith(TYPE_PREFIX) else SYMBOL_PREFIX if row[3].startswith(SYMBOL_PREFIX) else None
        if prefix is None:
            continue
        module = scalar(row[2], number, 1)
        feature = TYPE_FEATURE if prefix == TYPE_PREFIX else SYMBOL_FEATURE
        if row[1] != "module" or model.capabilities.get(module, {}).get(feature) not in ("partial", "available"):
            raise ValueError("Unavailable or unowned declaration recovery")
        key = module, prefix
        if row[3] == prefix + "-count":
            if key in counts:
                raise ValueError("Repeated declaration recovery count")
            counts[key] = scalar(row[4], number, 0, 100000)
            continue
        groups = types if prefix == TYPE_PREFIX else symbols
        group = groups.setdefault(module, [])
        parts = row[3].split(":")
        ordinal = scalar(parts[1], number, 1, 100000) if len(parts) == 2 else 0
        if len(parts) != 2 or parts[0] != prefix or ordinal != len(group) + 1:
            raise ValueError("Malformed or unordered declaration recovery identity")
        fields = row[4].split("\t")
        if prefix == TYPE_PREFIX:
            if len(fields) != 15 or fields[0] not in ("replacement", "pointer", "discarded"):
                raise ValueError("Malformed recovery type choice")
            dtype = scalar(fields[1], number, 0, 4294967295)
            if fields[0] == "discarded" and dtype != 2147483648:
                raise ValueError("Discarded recovery type is not the invalid sentinel")
            if fields[0] == "pointer" and dtype & 480 == 0:
                raise ValueError("Recovery pointer choice has no pointer depth")
            fields[2:] = site(fields[2:], model, module, number, unescape)
        else:
            if len(fields) != 23 or fields[0] not in ("mock-parameter", "generated-name") or fields[1] not in ("0", "1"):
                raise ValueError("Malformed recovery declaration snapshot")
            raw_class = scalar(fields[2], number)
            dtype = scalar(fields[3], number, 0, 4294967295)
            scalar(fields[4], number, -9223372036854775808, 9223372036854775807)
            for value in fields[5:8]:
                scalar(value, number, -9223372036854775808, 18446744073709551615)
            fields[8:10] = [unescape(value) for value in fields[8:10]]
            if any("\0" in value for value in fields[8:10]):
                raise ValueError("Null byte in recovered declaration name")
            if fields[1] == "0":
                if raw_class or dtype != 2147483648 or fields[4:8] != ["0"] * 4 or fields[9]:
                    raise ValueError("Absent recovered symbol invents a payload")
            elif fields[0] != "mock-parameter" or raw_class != 4:
                raise ValueError("Recovered parameter has another native symbol class")
            if fields[0] == "generated-name" and (fields[1] != "0" or not fields[8]):
                raise ValueError("Generated recovery name invents a symbol binding")
            fields[10:] = site(fields[10:], model, module, number, unescape)
        group.append(fields)
    for module, capabilities in model.capabilities.items():
        for feature, prefix, groups in ((TYPE_FEATURE, TYPE_PREFIX, types), (SYMBOL_FEATURE, SYMBOL_PREFIX, symbols)):
            if capabilities.get(feature) in ("partial", "available") and counts.get((module, prefix)) != len(groups.get(module, ())):
                raise ValueError("Declaration recovery coverage is incomplete")
    model.type_recovery, model.symbol_recovery = types, symbols
    affected = {int(fields[5]) for group in types.values() for fields in group if int(fields[5])}
    affected.update(int(fields[13]) for group in symbols.values() for fields in group if int(fields[13]))
    for (domain, identity), statement in model.statement_owners.items():
        if domain == "expression" and statement in affected:
            model.expression_validity[identity] = "recovery-affected"

# end of semantic_declaration_recovery.py
