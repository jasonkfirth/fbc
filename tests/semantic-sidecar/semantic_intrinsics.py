"""Project: FreeBASIC semantic sidecar
File: semantic_intrinsics.py
Purpose: Validate native intrinsic grammar selections and return snapshots.
Responsibilities: Route vocabulary, parser outcomes, source ownership and count closure.
This file intentionally does NOT infer execution or selected calls from grammar attempts.
"""

FEATURE = "native-intrinsic-dispatch"
PREFIX = "native-intrinsic-dispatch"
ROUTES = {
    "statement": {
        "cGotoStmt", "cPrintStmt", "cDataStmt", "cEraseStmt", "cSwapStmt",
        "cLineInputStmt", "cInputStmt", "cPokeStmt", "cFileStmt", "cOnStmt",
        "cWriteStmt", "cErrorStmt", "cErrSetStmt", "cViewStmt", "cMidStmt",
        "cLRSetStmt", "cWidthStmt", "cColorStmt", "cComment", "cVALISTStmt",
        "hParseMusic", "hParseSfx", "hParseMidi", "hParseDevice", "hParseCapture",
        "cGfxPSet", "cGfxLine", "cGfxCircle", "cGfxPaint", "cGfxDraw",
        "cGfxView", "cGfxPalette", "cGfxPut", "cGfxGet", "cGfxScreen",
    },
    "function": {
        "cSerializationFunct", "cMKXFunct", "cCVXFunct", "cStringFunct", "cMathFunct",
        "cPeekFunct", "cArrayFunct", "cFileFunct", "cErrorFunct", "cStrIdxOrMemberDeref",
        "cVAFunct", "cVALISTFunct", "cTypeConvExpr", "cViewStmt", "cWidthStmt",
        "cColorStmt", "cScreenFunct", "cThreadCallFunc", "hParseMusic", "hParseSfx",
        "hParseMidi", "hParseDevice", "hParseCapture", "cGfxPoint", "cGfxImageCreate",
    },
}


def scalar(text, number, maximum=2147483647):
    value = number(text, 0)
    if str(value) != text or value > maximum:
        raise ValueError("Noncanonical or oversized intrinsic dispatch scalar")
    return value


def point(fields, model, module, number, unescape):
    filename = unescape(fields[0])
    if "\0" in filename:
        raise ValueError("Null byte in intrinsic source filename")
    values = [scalar(value, number) for value in fields[1:]]
    source, line, column, raw_line, raw_column, physical = values
    if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
        raise ValueError("Intrinsic dispatch has a dangling source context")
    if physical not in (0, 1):
        raise ValueError("Invalid intrinsic physical source flag")
    if physical:
        if not source or not filename or raw_line < 1:
            raise ValueError("Physical intrinsic point has no source")
    elif raw_line or raw_column:
        raise ValueError("Nonphysical intrinsic point invents physical coordinates")
    return [filename, *fields[1:]]


def validate_intrinsics(model, number, unescape, node_kinds):
    groups, counts = {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number)
        if row[1] != "module" or not module or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or unowned intrinsic dispatch observation")
        if row[3] == PREFIX + "-count":
            if module in counts:
                raise ValueError("Repeated intrinsic dispatch count")
            counts[module] = scalar(row[4], number, 100000)
            continue
        group = groups.setdefault(module, [])
        parts = row[3].split(":")
        if (len(parts) != 2 or parts[0] != PREFIX
                or scalar(parts[1], number, 100000) != len(group) + 1):
            raise ValueError("Malformed or unordered intrinsic dispatch identity")
        fields = row[4].split("\t")
        if len(fields) != 26 or fields[0] not in ROUTES or fields[1] not in ROUTES[fields[0]]:
            raise ValueError("Unknown intrinsic dispatch route or payload")
        for index in (2, 3, 4, 5, 7, 8, 9, 10, 11):
            scalar(fields[index], number, 4294967295 if index == 11 else 2147483647)
        if fields[3] not in ("0", "1") or fields[9] not in ("0", "1"):
            raise ValueError("Invalid intrinsic dispatch result flag")
        token, accepted, before, after = map(int, fields[2:6])
        if after < before:
            raise ValueError("Intrinsic dispatch error counter moved backwards")
        expected = "recovered" if after != before else "accepted" if accepted else "declined"
        if fields[6] != expected:
            raise ValueError("Intrinsic outcome contradicts the native parser return")
        statement, configuration, present, raw_class, dtype = map(int, fields[7:12])
        if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
            raise ValueError("Intrinsic dispatch has a dangling statement")
        if ((configuration and model.configurations.get(configuration) != module)
                or (not configuration and not (model.expressions_only or model.bindings_only))):
            raise ValueError("Intrinsic dispatch has a dangling configuration")
        if fields[0] == "statement" and present or fields[0] == "function" and present != accepted:
            raise ValueError("Intrinsic dispatch has a contradictory root snapshot")
        if raw_class >= len(node_kinds):
            raise ValueError("Intrinsic result has an unknown native AST class")
        if not present and (raw_class or dtype != 2147483648):
            raise ValueError("Absent intrinsic root invents a payload")
        fields[12:19] = point(fields[12:19], model, module, number, unescape)
        fields[19:26] = point(fields[19:26], model, module, number, unescape)
        group.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) == "available" and counts.get(module) != len(groups.get(module, ())):
            raise ValueError("Intrinsic dispatch coverage is incomplete")
    model.intrinsic_dispatch = groups

# end of semantic_intrinsics.py
