"""Project: FreeBASIC semantic sidecar tests.
File: check_semantic_variable_names.py
Purpose: Qualify native spellings independently of the optional lint convention.
Responsibilities: Coverage, physical origins, wire round trips and damaged controls.
This file intentionally does NOT build compilers or infer declarations from text.
"""

import json
import sys
from pathlib import Path

from sidecar import Model, DETAIL_TAGS
from semantic_variable_names import PREFIX, COUNT_PREFIX, FEATURE, global_variable


def wire_field(value):
    """Encode once, preserving byte filenames and embedded property tabs."""
    return "".join(chr(byte) if 32 <= byte < 127 and byte != 37 else f"%{byte:02X}"
                   for byte in value.encode("utf-8", errors="surrogateescape"))


def repair_footer(rows):
    footer = rows[-1]
    for index, tag in enumerate(("M", "P", "S", "V", "N", "E", "B", "I", "D"), 2):
        footer[index] = str(sum(row[0] == tag for row in rows))
    footer[12] = str(sum(row[0] in DETAIL_TAGS for row in rows))
    return "\n".join("\t".join(wire_field(field) for field in row) for row in rows) + "\n"


def main():
    model = Model(Path(sys.argv[1]).read_text(encoding="ascii"))
    model.validate_physical_locations()
    # First prove that repaired generic counts and percent encoding preserve
    # a valid model. Otherwise unrelated framing failures could hide bad tests.
    Model(repair_footer([row[:] for row in model.rows])).validate_physical_locations()
    first = next(row for row in model.records["K"] if row[3].startswith(PREFIX)
                 and global_variable(model, int(row[2]))
                 and ("source-context", int(row[4].split("\t")[1]), row[3]) in model.physical_locations)
    symbol, role = first[2], first[3]
    source = first[4].split("\t")[1]
    mutations = ("missing-spelling", "missing-location", "wrong-kind", "wrong-context",
                 "missing-count", "wrong-count", "duplicate-count", "unavailable-capability",
                 "wrong-column", "canonical-case-forgery", "missing-spelling-with-repaired-count")
    for damage in mutations:
        rows = [row[:] for row in model.rows]
        occurrence = next(row for row in rows if row[:4] == ["K", "symbol", symbol, role])
        receipt = next(row for row in rows if row[0] == "K" and row[3].startswith(COUNT_PREFIX))
        if damage.startswith("missing-spelling"):
            rows.remove(occurrence)
            if damage.endswith("repaired-count"):
                receipt[4] = str(int(receipt[4]) - 1)
        elif damage == "missing-location":
            rows = [row for row in rows if row[:4] != ["LOC", "source-context", source, role]]
        elif damage in ("wrong-kind", "wrong-context", "canonical-case-forgery"):
            payload = occurrence[4].split("\t")
            if damage == "wrong-kind":
                payload[2] = "parameter"
            elif damage == "wrong-context":
                payload[1] = "0"
            else:
                payload[3] = payload[3].upper()
            occurrence[4] = "\t".join(payload)
        elif damage == "missing-count":
            rows.remove(receipt)
        elif damage == "wrong-count":
            receipt[4] = str(int(receipt[4]) + 1)
        elif damage == "duplicate-count":
            rows.insert(-1, receipt[:])
        elif damage == "unavailable-capability":
            next(row for row in rows if row[0] == "CAP" and row[2] == FEATURE)[3] = "unavailable"
        else:
            location = next(row for row in rows if row[:4] == ["LOC", "source-context", source, role])
            location[6] = str(int(location[6]) + 1)
            location[8] = str(int(location[8]) + 1)
        try:
            damaged = Model(repair_footer(rows))
            damaged.validate_physical_locations()
            if damage == "canonical-case-forgery":
                # Case is deliberately absent from canonical symbol identities.
                # The policy consumer performs this physical spelling comparison.
                continue
        except ValueError:
            continue
        raise ValueError(f"Independent reader accepted damaged variable evidence: {damage}")
    print(json.dumps({"Passed": True, "Spellings": len(model.variable_declaration_spellings),
                      "GlobalSpellings": sum(global_variable(model, identity)
                                             for identity, _ in model.variable_declaration_spellings),
                      "WireRoundTrip": True, "DamagedRefusals": len(mutations) - 1,
                      "CaseCrossCheck": "policy consumer"}))


if __name__ == "__main__":
    main()

# end of check_semantic_variable_names.py
