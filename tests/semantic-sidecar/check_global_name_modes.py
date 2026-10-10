"""Project: FreeBASIC semantic sidecar tests.
File: check_global_name_modes.py
Purpose: Independently measure accepted spellings in full and reduced models.
Responsibilities: Native counts, advertised capabilities and model readability.
This file intentionally does NOT infer a missing physical macro spelling.
"""
import json
import sys
from pathlib import Path

from sidecar import Model
from semantic_variable_names import FEATURE

model = Model(Path(sys.argv[1]).read_text(encoding="ascii"),
              expressions_only=sys.argv[2] == "expressions", bindings_only=sys.argv[2] == "bindings")
model.validate_physical_locations()
available = model.capabilities[1].get(FEATURE)
count = len(model.variable_declaration_spellings)
if sys.argv[2] in ("bindings", "expressions") and (available != "unavailable" or count):
    raise ValueError("Reduced model incorrectly claims full accepted spelling coverage")
print(json.dumps({"Passed": True, "Capability": available, "Spellings": count}))

# end of check_global_name_modes.py
