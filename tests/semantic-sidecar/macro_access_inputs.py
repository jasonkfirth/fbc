"""Project: FreeBASIC semantic macro access tests
File: macro_access_inputs.py
Purpose: Qualify surviving source access associations without changing emission.
Responsibilities: Exact source identities, transport parity and capture exclusions.
This file intentionally does NOT infer associations from statement positions.
"""
from collections import Counter
import subprocess

from sidecar import Model

FEATURE = "macro-source-access-node-bindings"
LINK = "macro-source-access-reference"
COUNT = "macro-source-access-node-count:"


def check_inputs(test):
    source = test.fixture("macro-access-nodes.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            emission = test.emission_path(source.stem, backend).read_bytes()
            _, path = test.invoke([source], backend=backend, extra=("-semantic-type-abi", "native", "-semantic-extra-facts", "macro-access"))
            model = Model.read(path)
            model.validate_physical_locations()
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            links = [row for row in model.records["K"] if row[3] == LINK]
            test.assertEqual(len(links), 3)
            roles = []
            for row in links:
                node = model.nodes[int(row[2])]
                symbol = int(node[9])
                role = "reference-" + row[4]
                test.assertIn(("symbol", symbol, role), model.macro_origins)
                access = model.properties["symbol", symbol]["macro-source-access:" + row[4]]
                roles.append((model.symbols[symbol][2].upper(), access))
            test.assertEqual(Counter(roles), Counter({("LOCALTEXT", "write"): 1,
                                                     ("OBJECTVALUE", "read"): 1,
                                                     ("TEXT", "write"): 1}))
            test.assertEqual([row[4] for row in model.records["K"] if row[3].startswith(COUNT)], ["3"])
            binary = test.working / ("macro-access-" + backend + ".fbcsem")
            result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix),
                                     "-i", str(test.root / "inc"), "-gen", backend, "-maxerr", "20", "-r",
                                     "-semantic-type-abi", "native", "-semantic-extra-facts", "macro-access", "-semantic-model", str(binary),
                                     "-o", str(test.emission_path(source.stem, backend)), str(source)],
                                    capture_output=True, timeout=120, check=False)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            test.assertEqual(Model.read(binary).rows, model.rows)
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            for mode, extra in (("full", ("-semantic-extra-facts", "none")),
                                ("bindings", ("-semantic-extra-facts", "macro-access")),
                                ("expressions", ("-semantic-extra-facts", "macro-access")),
                                ("full", ("-semantic-extra-facts", "macro-access", "-semantic-model-compact"))):
                _, reduced_path = test.invoke([source], backend=backend, mode=mode,
                                              extra=("-semantic-type-abi", "native", *extra))
                # These controls qualify this family's omission. Compact macro
                # assignments deliberately omit locations required by unrelated
                # full-model assignment validators, so inspect the emitted rows.
                reduced = [line.split("\t") for line in reduced_path.read_text(encoding="ascii").splitlines()]
                test.assertEqual([row[3] for row in reduced if row[:3] == ["CAP", "1", FEATURE]], ["unavailable"])
                test.assertFalse(any(row[0] == "K" and (row[3] == LINK or row[3].startswith(COUNT)) for row in reduced))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)

# end of macro_access_inputs.py
