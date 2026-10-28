"""Project: FreeBASIC semantic sidecar
File: semantic_skips.py
Purpose: Validate actual shared-parser token skipping observations.
Responsibilities: Delimiter policies, terminal states, counts and source ownership.
This file intentionally does NOT classify every skip as a source error.
"""

FEATURE = "native-token-skip-steps"
PREFIX = "native-token-skip"
# Native FB_TOKEN terminal values, separate from punctuation's ASCII tokens.
EOF, EOL = 256, 257
BOUNDARIES = {258, 259, 260}
REASONS = {"target", "end-of-line", "end-of-file", "statement-boundary", "comma"}


def validate_skips(model, number, unescape):
    groups, counts = {}, {}
    for row in model.records["K"]:
        if not row[3].startswith(PREFIX):
            continue
        module = number(row[2], 1)
        if row[1] != "module" or model.capabilities.get(module, {}).get(FEATURE) not in ("partial", "available"):
            raise ValueError("Unavailable or unowned native skip observation")
        if row[3] == PREFIX + "-count":
            if module in counts:
                raise ValueError("Repeated native skip count")
            counts[module] = number(row[4], 0)
            if str(counts[module]) != row[4] or counts[module] > 100000:
                raise ValueError("Noncanonical or oversized native skip count")
            continue
        parts = row[3].split(":")
        ordinal = number(parts[1], 1) if len(parts) == 2 else 0
        steps = groups.setdefault(module, [])
        if parts[0] != PREFIX or len(parts) != 2 or str(ordinal) != parts[1] or ordinal != len(steps) + 1 or ordinal > 100000:
            raise ValueError("Malformed or unordered native skip identity")
        fields = row[4].split("\t")
        if len(fields) != 27 or fields[26] not in REASONS:
            raise ValueError("Malformed native skip observation")
        for index in range(26):
            if index in (12, 19):
                fields[index] = unescape(fields[index])
                if "\0" in fields[index]:
                    raise ValueError("Null byte in native skip source name")
                continue
            value = number(fields[index], -1 if index in (0, 5) else 0)
            maximum = (1 << 61) - 1 if index == 4 else 4294967295 if index == 2 else 2147483647
            if value > maximum or str(value) != fields[index]:
                raise ValueError("Noncanonical or oversized native skip scalar")
        for index in (1, 3, 6, 18, 25):
            if fields[index] not in ("0", "1"):
                raise ValueError("Invalid native skip flag")
        token, eat, flags, comma, skipped, stopped, consumed, nesting, statement, configuration, before, after = map(int, fields[:12])
        if after < before or (consumed and (not eat or token != stopped or not skipped)):
            raise ValueError("Contradictory native skip consumption or error count")
        if statement and (statement not in model.statements or int(model.statements[statement][7]) != module):
            raise ValueError("Native skip has a dangling statement owner")
        if ((configuration and model.configurations.get(configuration) != module)
                or (not configuration and not model.expressions_only)):
            raise ValueError("Native skip has a dangling configuration")
        for start in (12, 19):
            filename, source_id, line, column, raw_line, raw_column, physical = fields[start:start + 7]
            source = int(source_id)
            if source and (source not in model.source_contexts or int(model.source_contexts[source][4]) != module):
                raise ValueError("Native skip has a dangling source context")
            if physical == "1":
                if not source or not filename or int(raw_line) < 1:
                    raise ValueError("Physical native skip point has no source")
            elif (raw_line, raw_column) != ("0", "0"):
                raise ValueError("Nonphysical native skip invents physical coordinates")
        reason = fields[26]
        if reason == "end-of-file" and (stopped != EOF or consumed):
            raise ValueError("Native skip invents an EOF consumption")
        if reason == "end-of-line" and stopped != EOL:
            raise ValueError("Native skip has the wrong line boundary")
        if reason == "statement-boundary" and (stopped not in BOUNDARIES or token == EOL):
            raise ValueError("Native skip has the wrong statement boundary")
        if reason == "comma" and (stopped != 44 or nesting or (token != 44 and not comma)):
            raise ValueError("Native skip has an impossible comma boundary")
        if reason == "target" and (token != stopped or stopped in ({EOF, EOL, 44, 123} | BOUNDARIES)
                                   or (stopped in (41, 125) and nesting)):
            raise ValueError("Native skip has an impossible requested delimiter")
        steps.append(fields)
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) in ("partial", "available") and counts.get(module) != len(groups.get(module, ())):
            raise ValueError("Native skip coverage is incomplete")
    model.skip_steps = groups

# end of semantic_skips.py
