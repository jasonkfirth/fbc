"""Project: FreeBASIC semantic sidecar tests
File: semantic_primitive_types.py
Purpose: Validate original primitive type operands without parsing source text.
Responsibilities: Keyword/dtype agreement, module counts and exact source origins.
This file intentionally does NOT infer type uses from value types or names.
"""

from semantic_datatypes import primitive_name

FEATURE = 'accepted-primitive-type-inputs'
INPUT = 'primitive-type-input:'
COUNT = 'accepted-primitive-type-count:'
LAYOUT_FEATURE = 'accepted-primitive-type-layouts'
LAYOUT = 'primitive-type-layout:'
LIMIT = 1_000_000

# Named Y families. INTEGER<bits> keeps the accepted explicit width;
# UNSIGNED changes a signed family's selection without changing its token.
BASES = {
    'ANY': {'any'}, 'BOOLEAN': {'boolean'}, 'BYTE': {'byte', 'ubyte'}, 'UBYTE': {'ubyte'},
    'SHORT': {'short', 'ushort'}, 'USHORT': {'ushort'},
    'INTEGER': {'byte', 'ubyte', 'short', 'ushort', 'integer', 'uinteger', 'long', 'ulong',
                'longint', 'ulongint', 'integer128', 'uinteger128'},
    'UINTEGER': {'ubyte', 'ushort', 'uinteger', 'ulong', 'ulongint', 'uinteger128'},
    'LONG': {'long', 'ulong'}, 'ULONG': {'ulong'}, 'LONGINT': {'longint', 'ulongint'},
    'ULONGINT': {'ulongint'}, 'SINGLE': {'single'}, 'DOUBLE': {'double'},
    'STRING': {'string', 'fixed-string'}, 'ZSTRING': {'zstring'}, 'WSTRING': {'wstring'}, 'USTRING': {'ustring'},
}
DTYPE_MASK = 0x1FBFFFF  # Base, pointer, const, ByRef and native C-mangling bits.


def validate_primitive_types(model, number, subject_modules):
    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text or value > 2147483647:
            raise ValueError('Primitive type identity is not canonical or bounded')
        return value

    def available(module):
        return model.capabilities.get(module, {}).get(FEATURE) == 'available'

    keywords = None
    observed, closures, identities, keys = {}, {}, set(), set()
    layouts, used_layouts = {}, set()
    for row in model.records['K']:
        if not row[3].startswith(LAYOUT):
            continue
        module = subject_modules.get((row[1], int(row[2])))
        key = (row[1], int(row[2]), row[3][len(LAYOUT):])
        fields = row[4].split('\t')
        if (model.capabilities.get(module, {}).get(LAYOUT_FEATURE) != 'available'
                or not available(module) or key in layouts or len(fields) != 4
                or not fields[0] or len(fields[0]) > 512 or any(char in fields[0] for char in '\r\n\0`\\')
                or fields[1] not in ('value', 'pointer', 'string-descriptor', 'unsized')
                or not fields[2].isascii() or not fields[2].isdecimal() or len(fields[2]) > 20
                or str(int(fields[2])) != fields[2]):
            raise ValueError('Invalid primitive type layout')
        depth = decimal(fields[3])
        if (depth > 15 or (fields[1] == 'pointer') != (depth > 0)
                or (fields[1] == 'unsized') != (fields[2] == '0')):
            raise ValueError('Primitive type layout kind or width disagrees')
        decimal(row[3][len(LAYOUT):], 1)
        layouts[key] = fields
    locations, origins = set(), set()
    work_left = len(model.rows) * 8 + 8_000_000
    for row in model.records['K']:
        key, payload = row[3:5]
        if not key.startswith((INPUT, COUNT)):
            continue
        work_left -= len(key) + len(payload) + 1
        identity = decimal(row[2], 1)
        module = subject_modules.get((row[1], identity))
        symbol = model.symbols.get(identity)
        raw_key = tuple(row[1:4])
        if (work_left < 0 or len(key) > 64 or len(payload) > 160 or raw_key in keys
                or row[1] != 'symbol' or symbol is None or symbol[3] != '8'
                or symbol[11] != '0' or not available(module)):
            raise ValueError('Primitive type observation has an invalid owner or capability')
        keys.add(raw_key)
        if key.startswith(COUNT):
            count_module = decimal(key[len(COUNT):], 1)
            value = decimal(payload)
            if count_module != module or module in closures or value > LIMIT:
                raise ValueError('Primitive type module closure is invalid')
            closures[module] = value
            continue
        detail = decimal(key[len(INPUT):], 1)
        fields = payload.split('\t')
        if detail in identities or len(fields) != 5:
            raise ValueError('Primitive type identity or payload is invalid')
        identities.add(detail)
        layout_key = ('symbol', identity, str(detail))
        if model.capabilities.get(module, {}).get(LAYOUT_FEATURE) == 'available':
            if layout_key not in layouts:
                raise ValueError('Primitive type observation lacks its layout')
            used_layouts.add(layout_key)
        statement, source, token, dtype, ordinal = (
            decimal(text, 0 if index == 3 else 1) for index, text in enumerate(fields))
        if (statement not in model.statements or source not in model.source_contexts
                or subject_modules.get(('statement', statement)) != module
                or model.statements[statement][5] != str(source)
                or model.source_contexts[source][4] != str(module)
                or ordinal != observed.get(module, 0) + 1 or ordinal > LIMIT
                or len(identities) > LIMIT or token > 8192):
            raise ValueError('Primitive type statement/source/module ownership is inconsistent')
        if keywords is None:
            keywords = {}
            for snapshot in model.records['T']:
                if snapshot[3] != 'keyword' or snapshot[8] != snapshot[9]:
                    continue
                keyword = int(snapshot[1])
                spelling = snapshot[2].upper().removeprefix('__')
                keyword_module = subject_modules.get(('symbol', keyword))
                token_text = model.properties['symbol', keyword].get('keyword-token')
                if token_text is not None:
                    keywords[keyword_module, decimal(token_text, 1)] = spelling
        spelling = keywords.get((module, token))
        if ((dtype & ~DTYPE_MASK) or ((dtype & 0x1E0) >> 5) > 8
                or primitive_name(model, module, dtype) not in BASES.get(spelling, ())):
            raise ValueError('Primitive type keyword and selected dtype disagree')
        location_key, origin_key = ('source-context', source, key), ('symbol', identity, key)
        location = model.physical_locations.get(location_key)
        expansion = model.macro_origins.get(origin_key)
        if location is not None:
            if location_key in locations or location[4] != str(source):
                raise ValueError('Primitive type location is repeated or foreign')
            locations.add(location_key)
        if expansion is not None:
            invocation = model.macro_invocations.get(expansion)
            if origin_key in origins or invocation is None or invocation[4] != str(source):
                raise ValueError('Primitive type macro origin is repeated or foreign')
            origins.add(origin_key)
        # An unverified LOC still owns the physical opening, but carries no
        # editable byte range. Legacy or malformed source bytes must not
        # erase an otherwise accepted type observation.
        elif (location is None
              and model.capabilities.get(module, {}).get('macro-reference-origins') == 'available'):
            raise ValueError('Primitive type lacks a physical or macro opening')
        observed[module] = ordinal
    for module in model.capabilities:
        if available(module) and closures.get(module) != observed.get(module, 0):
            raise ValueError('Primitive type occurrence inventory is incomplete')
    if layouts.keys() != used_layouts:
        raise ValueError('Primitive type layout has no observation')
    for location_key in model.physical_locations:
        if location_key[2].startswith(INPUT) and location_key not in locations:
            raise ValueError('Primitive type location has no observation')
    for origin_key in model.macro_origins:
        if origin_key[2].startswith(INPUT) and origin_key not in origins:
            raise ValueError('Primitive type macro opening has no observation')


# end of semantic_primitive_types.py
