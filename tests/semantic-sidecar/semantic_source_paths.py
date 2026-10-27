"""Project: FreeBASIC semantic source revisions
File: semantic_source_paths.py
Purpose: Keep native Windows filename bytes readable without changing wire identity.
Responsibilities: Compiler-produced paths, both source encodings and stale rejection.
This file intentionally does NOT normalize serialized filenames or relax source hashes.
"""
from sidecar import Model, filesystem_path


def check_windows_source_paths(test) -> None:
    if not test.native_windows:
        test.skipTest("The native Windows ANSI fallback does not apply to this host")
    header = test.source("' Project: semantic source path controls\n' File: café.bi\n"+
        "' Purpose: Observe an actual included file revision.\n"+
        "' Responsibilities: Original native path bytes.\n"+
        "' This file intentionally does NOT declare runtime state.\n"+
        "' end of café.bi\n", "café.bi")
    original_bytes = header.read_bytes()
    changed_bytes = original_bytes.replace(b"Original", b"Modified")
    test.assertNotEqual(original_bytes, changed_bytes)
    test.assertEqual(len(original_bytes), len(changed_bytes))
    for ordinal, encoding in enumerate(("utf-8", "utf-8-sig")):
        name = f"native-path-{ordinal}.bas"
        source = test.source("' Project: semantic source path controls\n' File: "+name+"\n"+
            "' Purpose: Include a file whose name has native non-ASCII bytes.\n"+
            "' Responsibilities: Compiler-owned paths and immutable source revisions.\n"+
            "' This file intentionally does NOT execute code.\n"+
            "#include \"café.bi\"\n' end of "+name+"\n", name)
        source.write_bytes(source.read_text(encoding="utf-8").encode(encoding))
        _, path = test.invoke([source], extra=("-semantic-type-abi", "native"))
        model = Model.read(path)
        original_rows = [row[:] for row in model.rows]
        test.assertTrue(any(filesystem_path(row[2]).name == header.name for row in model.files.values()))
        model.validate_physical_locations()
        model.verified_source_contents()
        test.assertEqual(model.rows, original_rows, "Filesystem access changed a wire identity")
        # The edit keeps the byte count fixed. Reject its digest, independently
        # of size checks and of the previous successful verification call.
        try:
            header.write_bytes(changed_bytes)
            with test.assertRaisesRegex(ValueError, "stale"):
                model.verified_source_contents()
        finally:
            header.write_bytes(original_bytes)
        path.unlink()


# end of semantic_source_paths.py
