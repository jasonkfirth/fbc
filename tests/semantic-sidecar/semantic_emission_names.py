"""Project: FreeBASIC semantic sidecar
File: semantic_emission_names.py
Purpose: Validate backend-selected names separately from object-symbol evidence.
Responsibilities: Ownership, stages, explicit unknown states and counted closure.
This file intentionally does NOT derive linker names from source aliases.
"""

from collections import defaultdict

FEATURE = "selected-emission-names"
PREFIX = "emission-name:"
CLOSURE = "emission-name-count:"
LIMIT = 1000000
NAME_BYTES = 65536
REASONS = {"automatic-storage", "backend-tls", "c-local-static", "llvm-private",
           "backend-intrinsic", "assembler-quoted"}


def validate_emission_names(model, number, unescape, subject_modules):
    observations, counts = defaultdict(dict), {}
    selected = set()
    module = 0

    def decimal(text, minimum=0, maximum=LIMIT):
        value = number(text, minimum)
        if str(value) != text or value > maximum:
            raise ValueError("Emission name identity or count is not canonical or bounded")
        return value

    for row in model.rows:
        if row[0] == "M":
            module += 1
            continue
        if row[0] != "K" or not row[3].startswith("emission-name"):
            continue
        owner = decimal(row[2], 1, 4294967295)
        symbol = model.symbols.get(owner)
        if (row[1] != "symbol" or symbol is None or subject_modules.get(("symbol", owner)) != module
                or model.capabilities[module].get(FEATURE) != "available"):
            raise ValueError("Emission name has an unavailable or foreign symbol owner")
        if row[3].startswith(CLOSURE):
            if (row[3] != CLOSURE + str(module) or module in counts or symbol[3] != "8"
                    or symbol[11] != "0"):
                raise ValueError("Emission name closure has a repeated or foreign namespace owner")
            counts[module] = decimal(row[4])
            continue
        if not row[3].startswith(PREFIX):
            raise ValueError("Unknown emission name property")
        ordinal = decimal(row[3][len(PREFIX):], 1)
        if ordinal != len(observations[module]) + 1 or module in counts:
            raise ValueError("Emission name sequence is repeated, reordered or follows its closure")
        fields = row[4].split("\t")
        if len(fields) != 4 or symbol[3] not in ("1", "3"):
            raise ValueError("Invalid emission name shape or symbol class")
        stage, state, spelling, reason = fields
        if stage not in ("c", "llvm", "gas", "gas64", "object") or state not in ("selected", "unavailable", "none"):
            raise ValueError("Unknown emission name stage or state")
        backend = model.contexts[module - 1][5]
        expected_stage = "c" if backend in ("gcc", "clang") else backend
        if stage != "object" and stage != expected_stage:
            raise ValueError("Emission name belongs to another backend")
        spelling = unescape(spelling)
        if len(spelling.encode("utf-8", "surrogateescape")) > NAME_BYTES or "\0" in spelling:
            raise ValueError("Emission name is oversized or contains NUL")
        if (state == "selected" and (not spelling or reason)
                or state != "selected" and (spelling or reason not in REASONS or stage != "object")):
            raise ValueError("Emission name state disagrees with its spelling or reason")
        if state == "none" and (reason != "automatic-storage" or symbol[3] != "1"):
            raise ValueError("Non-object emission state does not describe automatic storage")
        if state == "unavailable":
            if (reason == "automatic-storage" or reason in ("backend-tls", "c-local-static") and symbol[3] != "1"
                    or reason == "c-local-static" and expected_stage != "c"
                    or reason == "llvm-private" and backend != "llvm"
                    or reason == "backend-intrinsic" and (symbol[3] != "3" or expected_stage not in ("c", "llvm"))
                    or reason == "assembler-quoted" and backend not in ("gas", "gas64")):
                raise ValueError("Unavailable object name reason contradicts its backend or symbol")
        key = module, owner, stage
        if key in selected:
            raise ValueError("Emission name repeats a symbol and stage")
        selected.add(key)
        signature = model.signatures.get(owner)
        if signature is not None and signature[2] == "procedure-pointer":
            raise ValueError("Procedure-pointer type invents an emitted symbol")
        observations[module][ordinal] = (owner, stage, state, spelling, reason)
    model.emission_names = {}
    for module, capabilities in model.capabilities.items():
        if capabilities.get(FEATURE) != "available":
            continue
        if module not in counts or len(observations[module]) != counts[module]:
            raise ValueError("Emission name count closure is missing or incomplete")
        model.emission_names[module] = tuple(observations[module].values())

# end of semantic_emission_names.py
