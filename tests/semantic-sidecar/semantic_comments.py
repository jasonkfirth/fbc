"""FreeBASIC sidecar: semantic_comments.py.

Validate accepted physical comment marker coverage and source ownership.
This module does not tokenize source text or choose a documentation policy.
"""


def validate_comments(model, number, subject_modules):
    """Each complete module has a count receipt, including quiet modules."""
    counts = {}
    for row in model.records["COM"]:
        identity, context = number(row[1], 1), number(row[2], 1)
        module = model.configurations.get(context)
        if module is None or row[3] not in ("apostrophe", "rem"):
            raise ValueError("Comment has no accepted kind or configuration")
        location = model.physical_locations.get(("comment", identity, "marker"))
        if location is None:
            raise ValueError("Comment has no physical marker")
        source = model.source_contexts[int(location[4])]
        width = 3 if row[3] == "rem" else 1
        if (int(source[4]) != module or location[11] not in ("mapped", "unverified")
                or location[5] != location[7]
                or int(location[8]) - int(location[6]) != width
                or (location[11] == "mapped" and int(location[10]) <= int(location[9]))
                or (location[11] == "unverified" and location[9:11] != ["-1", "-1"])):
            raise ValueError("Comment marker has a foreign source or invalid width")
        # An invalid encoded unit elsewhere on the line prevents mapping the
        # accepted marker to bytes. Keep coverage, with no editable byte span.
        counts[module] = counts.get(module, 0) + 1
    for module, capabilities in model.capabilities.items():
        if capabilities.get("accepted-line-comment-markers") != "available":
            if counts.get(module, 0):
                raise ValueError("Comment records lack native capability")
            continue
        receipts = [row for row in model.records["K"]
                    if row[3] == f"accepted-line-comment-count:{module}"]
        if (len(receipts) != 1 or receipts[0][1] != "symbol"
                or subject_modules.get(("symbol", int(receipts[0][2]))) != module
                or model.types.get(int(receipts[0][2]), [None] * 4)[3] != "namespace"
                or number(receipts[0][4], 0) != counts.get(module, 0)):
            raise ValueError("Accepted comment coverage count is missing or contradictory")

# end of semantic_comments.py
