"""FreeBASIC sidecar: check_semantic_comments.py.

Independently check native comment models and damaged coverage controls.
This controller does not build or install compilers, or implement lint policy.
"""

import json
import sys
from pathlib import Path

from sidecar import Model, DETAIL_TAGS


def repair_footer(rows):
    """Mutations repair generic counts so the specific contract must refuse."""
    footer = rows[-1]
    for index, tag in enumerate(("M", "P", "S", "V", "N", "E", "B", "I", "D"), 2):
        footer[index] = str(sum(row[0] == tag for row in rows))
    footer[12] = str(sum(row[0] in DETAIL_TAGS for row in rows))
    def wire_field(value):
        return "".join(chr(byte) if 32 <= byte < 127 and byte != 37 else f"%{byte:02X}"
                       for byte in value.encode("utf-8", errors="surrogateescape"))
    return "\n".join("\t".join(wire_field(field) for field in row) for row in rows) + "\n"


def main():
    text = Path(sys.argv[1]).read_text(encoding="ascii")
    model = Model(text)
    model.validate_physical_locations()
    # Prove the mutation framing first, so a broken encoder cannot make every
    # damaged contract appear to be rejected for the intended reason.
    Model(repair_footer([row[:] for row in model.rows])).validate_physical_locations()
    expected = {9, 10, 11, 12, 14, 17, 19, 21, 22, 34, 37}
    markers = {int(model.physical_locations["comment", int(row[1]), "marker"][5])
               for row in model.records["COM"]}
    if len(sys.argv) == 2 and (not expected <= markers or markers & {24, 26, 27, 28, 31, 32, 33, 42, 43}):
        raise ValueError(f"Native comment boundaries differ from fixture: {sorted(markers)}")
    first_id = model.records["COM"][0][1]
    mutations = ("missing-marker", "missing-comment", "wrong-kind", "wrong-context",
                 "missing-count", "wrong-count", "unavailable-capability")
    for damage in mutations:
        rows = [row[:] for row in model.rows]
        if damage == "missing-marker":
            rows = [row for row in rows if not (row[:3] == ["LOC", "comment", first_id])]
        elif damage == "missing-comment":
            rows = [row for row in rows if not (row[:2] == ["COM", first_id])]
        elif damage.startswith("wrong-") and damage != "wrong-count":
            row = next(row for row in rows if row[:2] == ["COM", first_id])
            row[3 if damage == "wrong-kind" else 2] = "block" if damage == "wrong-kind" else "0"
        elif damage in ("missing-count", "wrong-count"):
            receipt = next(row for row in rows if row[0] == "K" and row[3].startswith("accepted-line-comment-count:"))
            if damage == "missing-count":
                rows.remove(receipt)
            else:
                receipt[4] = str(int(receipt[4]) + 1)
        else:
            next(row for row in rows if row[0] == "CAP" and row[2] == "accepted-line-comment-markers")[3] = "unavailable"
        try:
            Model(repair_footer(rows))
        except ValueError:
            continue
        raise ValueError(f"Independent reader accepted damaged comment evidence: {damage}")
    print(json.dumps({"Passed": True, "Markers": len(model.records["COM"]),
                      "EmptyFixtureMarkers": len(expected), "DamagedRefusals": len(mutations)}))


if __name__ == "__main__":
    main()

# end of check_semantic_comments.py
