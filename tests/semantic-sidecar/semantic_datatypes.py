"""Project: FreeBASIC semantic sidecar
File: semantic_datatypes.py
Purpose: Interpret raw datatype families through each module's Y vocabulary.
Responsibilities: Frozen encoding vocabularies and named primitive families.
This file intentionally does NOT rewrite raw wire records or nominal identities.
"""

# The native enum inserted integer128 before SINGLE. Stable-v1 preserves the
# original slots and appends the two new integer widths. Both include private
# compiler slots, so deleting the final entry cannot mimic a shorter vocabulary.
NATIVE_PRIMITIVE_NAMES = ("any", "boolean", "byte", "ubyte", "zstring", "short", "ushort",
                          "wstring", "integer", "uinteger", "enum", "long", "ulong",
                          "longint", "ulongint", "integer128", "uinteger128",
                          "single", "double", "string", "fixed-string",
                          "va_list", "type", "namepace", "function", "fwdref", "pointer",
                          "xmmword", "ustring")
STABLE_PRIMITIVE_NAMES = ("any", "boolean", "byte", "ubyte", "zstring", "short", "ushort",
                          "wstring", "integer", "uinteger", "enum", "long", "ulong",
                          "longint", "ulongint", "single", "double", "string", "fixed-string",
                          "va_list", "type", "namepace", "function", "fwdref", "pointer",
                          "xmmword", "ustring", "integer128", "uinteger128")

INTEGER_NAMES = frozenset(("byte", "ubyte", "short", "ushort", "integer", "uinteger", "long", "ulong",
                           "longint", "ulongint", "integer128", "uinteger128"))
FLOAT_NAMES = frozenset(("single", "double"))
NUMERIC_NAMES = INTEGER_NAMES | FLOAT_NAMES
BOOLEAN_NUMERIC_NAMES = NUMERIC_NAMES | {"boolean"}
STRING_NAMES = frozenset(("zstring", "wstring", "string", "fixed-string", "ustring"))


def primitive_name(model, module, dtype):
    """Return the Y classifier without treating symbol display text as a type.

    Missing modules/slots have no family. Each calling validator rejects the
    unknown name in its own semantic context instead of leaking a KeyError.
    Pointer and qualifier fields remain the caller's responsibility.
    """
    if module is None or module < 1 or module > len(model.primitives):
        return ""
    info = model.primitives[module - 1].get(dtype & 31)
    return info[2] if info is not None else ""

# end of semantic_datatypes.py
