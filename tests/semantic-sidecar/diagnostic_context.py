"""Project: FreeBASIC semantic sidecar
File: diagnostic_context.py
Purpose: Validate accepted native reporter policy and parameter snapshots.
Responsibilities: Native flags, frame order, ownership and complete diagnostic closure.
This file intentionally does NOT infer a diagnostic context from console text.
"""

FEATURE = "reported-diagnostic-contexts"
COUNT_FEATURE = "reported-diagnostic-counts"
LEVEL_FEATURE = "reported-warning-levels"
PARAMETER_KINDS = frozenset(("none", "unknown-symbol", "constructor", "destructor", "operator", "procedure-pointer", "procedure"))


def policy_fields(value, number, *, warning_levels=False):
    fields = value.split("\t")
    if len(fields) != (13 if warning_levels else 12):
        raise ValueError("Malformed reported diagnostic context")
    limits = ((0, 1), (0, 4294967295), (0, 1), (0, 2147483647), (-1, 2147483647), (0, 2147483647),
              (0, 1), (0, 4096), (0, 2147483647), (0, 2147483647))
    for text, (minimum, maximum) in zip(fields, limits):
        parsed = number(text, minimum)
        if str(parsed) != text or parsed > maximum:
            raise ValueError("Diagnostic policy integer is not canonical or bounded")
    if not fields[10] or any(character in fields[10] for character in "\0\r\n\t") or fields[11] not in ("0", "1"):
        raise ValueError("Diagnostic target or lexer state is invalid")
    if fields[0] == "0" and fields[7] != "0":
        raise ValueError("Uninitialized reporter invents parameter contexts")
    if warning_levels and fields[12] not in ("-1", "1", "2", "3"):
        raise ValueError("Invalid native warning catalog level")
    return fields


def validate_warning_level(fields, severity):
    if len(fields) != 13:
        return
    level = int(fields[12])
    if severity != "warning" and level != -1:
        raise ValueError("Ordinary error invents a warning catalog level")
    if severity == "warning" and level != -1 and level < int(fields[4]):
        raise ValueError("Suppressed warning claims an accepted observation")


def parameter_fields(fields, ordinal, number):
    if len(fields) != 7 or fields[0] != str(ordinal) or fields[6] not in PARAMETER_KINDS:
        raise ValueError("Malformed or unordered diagnostic parameter context")
    for text in fields[1:3]:
        value = number(text, -1)
        if str(value) != text or value > 2147483647:
            raise ValueError("Diagnostic parameter integer is not canonical or bounded")
    if any("\0" in text for text in fields[3:6]):
        raise ValueError("Diagnostic parameter context contains a null name")
    if fields[6] == "none" and any(fields[4:6]):
        raise ValueError("Absent diagnostic procedure invents symbol names")
    return fields


def validate_diagnostic_contexts(model, number, subject_modules):
    from sidecar import unescape

    contexts, parameters, totals = {}, {}, {}
    for row in model.records["K"]:
        if row[1] == "module" and row[3].startswith(("native-expression-recovery", "native-token-skip", "native-type-recovery", "native-symbol-recovery", "native-statement-", "native-source-validity", "native-intrinsic-dispatch", "native-resolution", "native-lookup", "native-symbol-filter", "native-prototype", "native-qualified-filter", "native-procedure-state", "consumed-token", "extern-mangling", "native-default-type", "native-pragma", "native-evaluation")):
            continue
        if row[1] == "module":
            module = number(row[2], 1)
            if (model.capabilities.get(module, {}).get(COUNT_FEATURE) != "available"
                    or row[3] != COUNT_FEATURE or module in totals):
                raise ValueError("Unavailable or repeated diagnostic module summary")
            fields = row[4].split("\t")
            if len(fields) != 6 or fields[5] not in ("accepted", "rejected", "restarted", "aborted"):
                raise ValueError("Malformed diagnostic module summary")
            values = [number(value, -1 if index == 4 else 0) for index, value in enumerate(fields[:5])]
            if any(str(value) != text or value > 2147483647 for value, text in zip(values, fields)):
                raise ValueError("Diagnostic module count is not canonical or bounded")
            if not ((fields[5] in ("accepted", "restarted") and values[4] == 0)
                    or (fields[5] == "rejected" and values[4] > 0)
                    or (fields[5] == "aborted" and values[4] == -1)):
                raise ValueError("Contradictory diagnostic module outcome")
            totals[module] = fields
            continue
        if row[1] != "diagnostic":
            continue
        identity = number(row[2], 1)
        module = subject_modules.get(("diagnostic", identity))
        if identity not in model.diagnostics or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Reported diagnostic context is dangling or unavailable")
        if row[3] == "reported-context":
            if identity in contexts:
                raise ValueError("Repeated reported diagnostic context")
            levels = model.capabilities.get(module, {}).get(LEVEL_FEATURE) == "available"
            contexts[identity] = policy_fields(row[4], number, warning_levels=levels)
        elif row[3].startswith("reported-parameter:"):
            parts = row[3].split(":")
            ordinal = number(parts[1], 1) if len(parts) == 2 else 0
            frame_list = parameters.setdefault(identity, [])
            if len(parts) != 2 or str(ordinal) != parts[1] or ordinal != len(frame_list) + 1 or ordinal > 4096:
                raise ValueError("Diagnostic parameter context order is invalid")
            fields = row[4].split("\t")
            if len(fields) != 7:
                raise ValueError("Malformed diagnostic parameter field group")
            fields[3:6] = [unescape(value) for value in fields[3:6]]
            frame_list.append(parameter_fields(fields, ordinal, number))
        else:
            raise ValueError("Unknown diagnostic metadata role")
    model.diagnostic_contexts, model.diagnostic_parameters = contexts, parameters
    model.diagnostic_totals = totals
    observed = {}
    counters = {}
    for identity, diagnostic in model.diagnostics.items():
        module = subject_modules["diagnostic", identity]
        if model.capabilities.get(module, {}).get(FEATURE) != "available":
            continue
        context = contexts.get(identity)
        if context is None or int(context[7]) != len(parameters.get(identity, ())):
            raise ValueError("Reported diagnostic or parameter coverage is incomplete")
        if context[6] == "1" and diagnostic[2] != "warning":
            raise ValueError("Ordinary error claims warning promotion")
        validate_warning_level(context, diagnostic[2])
        actual = observed.setdefault(module, [0, 0, 0, 0])
        actual[0] += 1
        actual[1] += diagnostic[2] == "error"
        actual[2] += diagnostic[2] == "warning"
        actual[3] += context[6] == "1"
        count = counters.get(module, 0) + (context[6] == "1")
        if int(context[3]) != count:
            raise ValueError("Reported diagnostic counter lost an accepted event")
        # Native error 133 is the uncounted limit notice emitted after increment.
        count += diagnostic[2] == "error" and diagnostic[3] != "133"
        counters[module] = count
    for module, capabilities in model.capabilities.items():
        if (capabilities.get(LEVEL_FEATURE) == "available" or capabilities.get(COUNT_FEATURE) == "available") and capabilities.get(FEATURE) != "available":
            raise ValueError("Diagnostic coverage lacks its reporter context capability")
        if capabilities.get(COUNT_FEATURE) != "available":
            continue
        fields = totals.get(module)
        if (fields is None or fields[:4] != list(map(str, observed.get(module, [0, 0, 0, 0])))
                or (int(fields[4]) != -1 and int(fields[4]) != counters.get(module, 0))):
            raise ValueError("Reported diagnostic module coverage is incomplete")
        expected_outcome = "rejected" if module in model.recovered_modules else "accepted"
        if fields[5] != expected_outcome:
            raise ValueError("Native diagnostic outcome contradicts retained module validity")
    model.module_validity = {module: "recovery" if module in model.recovered_modules else "accepted"
                             for module in range(1, len(model.records["M"]) + 1)}
    model.expression_validity = {}
    for row in model.records["E"]:
        identity = number(row[1], 1)
        module = subject_modules["expression", identity]
        validity = "accepted"
        if module in model.recovered_modules:
            statement = model.statement_owners.get(("expression", identity))
            ending = model.statement_endings.get(statement)
            validity = "provisional"
            if ending is None or ending[3] != "parsed":
                validity = "recovery-affected"
        model.expression_validity[identity] = validity

# end of diagnostic_context.py
