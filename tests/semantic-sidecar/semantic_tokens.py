"""Project: FreeBASIC semantic sidecar
File: semantic_tokens.py
Purpose: Validate committed parser-token payload and provenance.
Responsibilities: Active lexer unions, copied text, coordinates and actual lookup links.
This file intentionally does NOT tokenize source or infer reference bindings.
"""

import re
from semantic_lookup import scalar, text_field
from semantic_datatypes import NATIVE_PRIMITIVE_NAMES, STABLE_PRIMITIVE_NAMES

FEATURE = "parser-consumed-tokens"
PREFIX = "consumed-token"
STABLE_BASE = {index: STABLE_PRIMITIVE_NAMES.index(name) for index, name in enumerate(NATIVE_PRIMITIVE_NAMES)}


def validate_tokens(model, number, unescape):
    tokens, policies, summaries = {}, {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = scalar(row[2], number, 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) != "available":
            raise ValueError("Unavailable or foreign consumed-token payload")
        if model.expressions_only or model.bindings_only:
            raise ValueError("Reduced model invents unavailable consumed-token payload")
        if row[3] == PREFIX + "-policy":
            if module in policies or module in summaries:
                raise ValueError("Repeated or late consumed-token policy")
            fields = row[4].split("\t")
            if len(fields) != 2:
                raise ValueError("Malformed consumed-token policy")
            limit = scalar(fields[0], number, 1, 1048576)
            wide = scalar(fields[1], number, 1, 31)
            policies[module] = limit, wide
            continue
        if row[3] == PREFIX + "-count":
            if module in summaries:
                raise ValueError("Repeated consumed-token summary")
            summaries[module] = scalar(row[4], number, 0, 1000000)
            continue
        module_tokens = tokens.setdefault(module, [])
        parts = row[3].split(":")
        if (module in summaries or len(parts) != 2 or parts[0] != PREFIX
                or scalar(parts[1], number, 1, 1000000) != len(module_tokens) + 1):
            raise ValueError("Consumed-token identities have gaps or follow their summary")
        fields = row[4].split("\t")
        if len(fields) != 43:
            raise ValueError("Malformed consumed-token payload")
        for index in (0, 2, 4):
            scalar(fields[index], number, 0, 4294967295)
        kind = scalar(fields[1], number, 0, 7)
        for index in (3, 8, 17, 20, 25, 28, 30, 35, 36, 40, 41):
            scalar(fields[index], number, 0, 1)
        for index in (5, 9, 10, 11, 13, 14, 15, 16, 18, 19, 21, 22, 23, 24, 27, 29, 34, 37, 38, 39, 42):
            scalar(fields[index], number)
        scalar(fields[33], number, -1, 1048576)
        for index in (7, 12, 31, 32):
            # Token byte payloads can include NUL. Coordinate/name fields
            # retain the ordinary byte-safe text restriction.
            fields[index] = unescape(fields[index]) if index == 7 else text_field(fields[index], unescape)
        statement, context, source = map(int, fields[9:12])
        if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
            raise ValueError("Consumed token has another statement owner")
        if not context or model.configurations.get(context) != module:
            raise ValueError("Consumed token has another configuration owner")
        if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
            raise ValueError("Consumed token has another source occurrence")
        if fields[18] != "0":
            invocation = model.macro_invocations.get(int(fields[18]))
            occurrence = model.source_contexts.get(int(invocation[4])) if invocation else None
            if occurrence is None or int(occurrence[4]) != module:
                raise ValueError("Consumed token has no actual module-owned macro expansion")
        if fields[25] == "1" and (not source or int(fields[21]) < 1 or int(fields[23]) < int(fields[21])):
            raise ValueError("Consumed token has no valid original-input coordinates")
        if kind in (0, 1, 2):
            if fields[26] != "period-position" or fields[28] != "1" or fields[30] != "0" or fields[40] != "0":
                raise ValueError("Identifier token invents another active union")
            if int(fields[27]) > int(fields[5]):
                raise ValueError("Identifier token period position exceeds its text")
        elif kind == 3:
            if fields[26] != "numeric-suffix-present" or fields[30] != "1" or fields[28] != "0" or fields[40] != "0" or fields[27] not in ("0", "1"):
                raise ValueError("Numeric token reads an inactive lexer union")
        elif kind == 4:
            if fields[26] != "string-escapes-present" or fields[40] != "1" or fields[28] != "0" or fields[30] != "0" or fields[27] not in ("0", "1"):
                raise ValueError("String token reads an inactive lexer union")
        elif fields[26:31] != ["none", "0", "0", "0", "0"] or fields[40] != "0":
            raise ValueError("Nonpayload token invents an active lexer union")
        if (fields[3] == "1") != (kind <= 4) or fields[3] == "0" and fields[4] != "2147483648":
            raise ValueError("Lexer discriminator is confused with a semantic datatype")
        if fields[28] == "0" and fields[29] != "0":
            raise ValueError("Token invents an uninitialized identifier suffix slot")
        if fields[30] == "0" and (any(fields[index] for index in (31, 32)) or any(fields[index] != "0" for index in range(33, 40))):
            raise ValueError("Nonnumeric token invents numeric observation storage")
        if fields[30] == "1" and (len(fields[31]) > 3 or len(fields[32]) > 1024):
            raise ValueError("Consumed numeric observation exceeds its native fixed buffers")
        if fields[40] == "0" and fields[41] != "0":
            raise ValueError("Nonstring token invents literal truncation")
        if int(fields[42]):
            lookup = model.lookup_queries.get((module, int(fields[42])))
            if lookup is None or lookup["end"] is None:
                raise ValueError("Consumed token has no completed actual lookup")
        module_tokens.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        if module not in policies or summaries.get(module) != len(tokens.get(module, ())):
            raise ValueError("Consumed-token terminal coverage is incomplete")
        limit, wide = policies[module]
        for fields in tokens.get(module, ()):
            if fields[3] == "1":
                raw = int(fields[2])
                stable = capabilities.get("stable-primitive-type-abi-v1")
                if stable == "available":
                    # Native base and builtin-mangling subtype are separate
                    # five-bit fields; retain pointer/const/ByRef bits exactly.
                    base, mangled = raw & 31, (raw >> 20) & 31
                    expected = ((raw & ~(31 | (31 << 20))) | STABLE_BASE.get(base, base)
                                | (STABLE_BASE.get(mangled, mangled) << 20))
                elif stable == "unavailable":
                    expected = raw
                else:
                    raise ValueError("Consumed datatype slot has no declared raw ABI")
                if int(fields[4]) != expected:
                    raise ValueError("Consumed datatype slot contradicts its raw/wire ABI mapping")
            length = int(fields[5])
            if length > limit:
                raise ValueError("Consumed-token text exceeds the native token buffer")
            if fields[6] == "wide-units":
                if fields[1] != "4" or int(fields[2]) != wide or len(fields[7]) != length * 8 or re.fullmatch(r"[0-9A-F]*", fields[7]) is None:
                    raise ValueError("Consumed wide token has invalid native code units")
            elif fields[6] != "bytes" or len(fields[7].encode("utf-8", errors="surrogateescape")) != length:
                raise ValueError("Consumed narrow token byte count contradicts its native payload")
    model.consumed_tokens = tokens
    model.consumed_token_policy = policies

# end of semantic_tokens.py
