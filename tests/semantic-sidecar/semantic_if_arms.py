"""Project: FreeBASIC semantic sidecar
File: semantic_if_arms.py
Purpose: Validate accepted IF arm roles and direct statement membership.
Responsibilities: Complete constructs, independent markers, counts and owners.
This file intentionally does NOT infer arm boundaries from source ranges.
"""

PREFIXES = ('if-construct:', 'if-arm:', 'if-arm-end:', 'if-arm-statement:',
            'if-arm-transfer:', 'if-end:')


def validate_if_arms(model, number, subject_modules):
    properties, markers, modes, arms, counts, members, transfers, endings = {}, set(), {}, {}, {}, {}, {}, {}
    work_left = 8000000

    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text:
            raise ValueError('IF arm identity is not canonical')
        return value

    order = {}
    for row in model.records['K']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('IF arm property work budget exceeded')
        if not row[3].startswith(PREFIXES):
            continue
        work_left -= len(row[3]) + len(row[4])
        if work_left < 0:
            raise ValueError('IF arm property work budget exceeded')
        owner = decimal(row[2], 1)
        module = subject_modules.get(('symbol', owner))
        if (row[1] != 'symbol' or owner not in model.symbols or model.symbols[owner][3] != '3'
                or model.capabilities.get(module, {}).get('if-arm-inputs') != 'available'
                or (owner, row[3]) in properties):
            raise ValueError('IF arm property is repeated, unavailable or lacks its procedure')
        parts = row[3].split(':')
        kind, identity = parts[0], decimal(parts[1], 1)
        expected_parts = 3 if kind in ('if-arm', 'if-arm-end') else 4 if kind == 'if-arm-transfer' else 2
        if len(parts) != expected_parts:
            raise ValueError('IF arm property key has an invalid shape')
        ordinal = decimal(parts[2], 1) if len(parts) >= 3 else 0
        item = decimal(parts[3], 1) if len(parts) == 4 else 0
        fields = row[4].split('\t')
        expected_fields = 2 if kind in ('if-construct', 'if-arm', 'if-end') else 3 if kind == 'if-arm-statement' else 1
        if len(fields) != expected_fields:
            raise ValueError('IF arm payload has an invalid shape')
        properties[owner, row[3]] = row
        order[owner, row[3]] = len(order)
        if kind == 'if-arm-statement':
            if identity in members:
                raise ValueError('IF statement belongs to more than one arm')
            members[identity] = (owner, decimal(fields[0], 1), decimal(fields[1], 1), decimal(fields[2], 1))
        else:
            block = model.constructs.get(identity)
            if (block is None or block[5] != 'if' or decimal(block[4], 1) != owner
                    or int(model.source_contexts[int(block[6])][4]) != module):
                raise ValueError('IF arm property belongs to another construct or module')
            if kind == 'if-construct':
                header = decimal(fields[0], 1)
                if fields[1] not in ('inline', 'block') or header != int(block[3]):
                    raise ValueError('IF mode differs from its opening header')
                start = model.statements.get(header)
                if (start is None or int(start[9]) != 266 or int(start[4]) != owner
                        or int(start[7]) != module or model.statement_endings[header][2:4] != ['compound', 'parsed']):
                    raise ValueError('IF mode has no accepted original header')
                modes[identity] = (owner, header, fields[1], module)
            elif kind == 'if-arm':
                arms[identity, ordinal] = (fields[0], decimal(fields[1]))
            elif kind == 'if-arm-end':
                counts[identity, ordinal] = decimal(fields[0])
            elif kind == 'if-arm-transfer':
                target = decimal(fields[0], 1)
                if target not in model.symbols or model.symbols[target][3] != '7' or subject_modules['symbol', target] != module:
                    raise ValueError('IF transfer has no selected label in its module')
                transfers[identity, ordinal, item] = target
            elif kind == 'if-end':
                endings[identity] = (decimal(fields[0], 1), decimal(fields[1]))

    for row in model.records['H']:
        work_left -= 1 + len(row[5])
        if work_left < 0:
            raise ValueError('IF arm marker work budget exceeded')
        role = row[5]
        if role in ('if-construct', 'if-end', 'if-arm-statement'):
            key = role + ':' + row[6]
        elif role.startswith(('if-arm:', 'if-arm-end:', 'if-arm-transfer:')):
            kind, suffix = role.split(':', 1)
            key = kind + ':' + row[6] + ':' + suffix
        else:
            continue
        owner = decimal(row[2], 1)
        decimal(row[6], 1)
        if (row[1] != 'symbol' or row[3] != 'symbol' or row[4] != row[2]
                or (owner, key) not in properties or (owner, key) in markers):
            raise ValueError('IF arm marker is repeated or lacks its property')
        markers.add((owner, key))
    if set(properties) != markers:
        raise ValueError('IF arm property lacks its independent occurrence marker')

    locations = {}
    for row in model.records['LOC']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('IF arm location work budget exceeded')
        if row[1] != 'construct' or not row[3].startswith('if-arm:'):
            continue
        identity, ordinal = decimal(row[2], 1), decimal(row[3][7:], 1)
        key = identity, ordinal
        if key not in arms or key in locations or identity not in modes:
            raise ValueError('IF arm location is repeated or lacks its accepted arm')
        if int(model.source_contexts[int(row[4])][4]) != modes[identity][3]:
            raise ValueError('IF arm location belongs to another module')
        locations[key] = row

    macro_origins = {}
    for row in model.records['MR']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('IF arm macro-origin work budget exceeded')
        if row[1] != 'construct' or not row[4].startswith('if-arm:'):
            continue
        identity, ordinal = decimal(row[2], 1), decimal(row[4][7:], 1)
        key = identity, ordinal
        if key not in arms or key in macro_origins or identity not in modes:
            raise ValueError('IF arm macro origin is repeated or lacks its accepted arm')
        invocation = model.macro_invocations[int(row[3])]
        if int(model.source_contexts[int(invocation[4])][4]) != modes[identity][3]:
            raise ValueError('IF arm macro origin belongs to another module')
        macro_origins[key] = row
    if set(arms) != set(counts) or set(arms) != (set(locations) | set(macro_origins)):
        raise ValueError('IF arm lacks its closing count or opening token')
    if set(modes) != set(endings):
        raise ValueError('IF mode lacks its complete arm count')
    by_construct = {}
    for (identity, ordinal), (role, header) in arms.items():
        work_left -= 1
        if identity not in modes or work_left < 0:
            raise ValueError('IF arm lacks its complete mode')
        owner, opening, mode, module = modes[identity]
        if role not in ('then', 'elseif', 'else') or (ordinal == 1) != (role == 'then'):
            raise ValueError('IF arm roles are invalid or out of order')
        by_construct.setdefault(identity, []).append((ordinal, role))
        if role == 'then':
            if header != opening:
                raise ValueError('THEN arm differs from its original IF header')
        elif mode == 'inline':
            if role != 'else' or header != 0:
                raise ValueError('Inline arm invents a statement header')
        else:
            start = model.statements.get(header)
            if (start is None or int(start[3]) != identity or int(start[4]) != owner or int(start[7]) != module
                    or int(start[9]) != (269 if role == 'elseif' else 268)
                    or model.statement_endings[header][2:4] != ['compound', 'parsed']):
                raise ValueError('IF arm lacks its accepted alternative header')

    items, headers = {}, {header for role, header in arms.values() if role != 'then' and header}
    for statement, (owner, identity, ordinal, item) in members.items():
        work_left -= 1
        start = model.statements.get(statement)
        if (work_left < 0 or identity not in modes or (identity, ordinal) not in arms
                or owner != modes[identity][0] or start is None or int(start[3]) != identity
                or int(start[4]) != owner or int(start[7]) != modes[identity][3]
                or model.statement_endings[statement][3] != 'parsed' or statement in headers):
            raise ValueError('IF statement has no accepted direct arm ownership')
        key = identity, ordinal, item
        if key in items:
            raise ValueError('IF arm item is repeated')
        items[key] = statement
        suffix = str(identity) + ':' + str(ordinal)
        if not order[owner, 'if-arm:' + suffix] < order[owner, 'if-arm-statement:' + str(statement)] < order[owner, 'if-arm-end:' + suffix]:
            raise ValueError('IF body receipt is outside its accepted arm')
    for key, target in transfers.items():
        work_left -= 1
        identity, ordinal, item = key
        if (work_left < 0 or identity not in modes or modes[identity][2] != 'inline'
                or (identity, ordinal) not in arms or item != 1 or counts[identity, ordinal] != 1 or key in items):
            raise ValueError('IF numeric-label item is repeated or lacks its inline arm')
        owner = modes[identity][0]
        suffix = str(identity) + ':' + str(ordinal)
        if not order[owner, 'if-arm:' + suffix] < order[owner, 'if-arm-transfer:' + suffix + ':1'] < order[owner, 'if-arm-end:' + suffix]:
            raise ValueError('IF transfer receipt is outside its accepted arm')
        items[key] = 0

    item_counts = {}
    for identity, ordinal, item in items:
        work_left -= 1
        if work_left < 0 or item != item_counts.get((identity, ordinal), 0) + 1 or item > counts[identity, ordinal]:
            raise ValueError('IF item exceeds its accepted arm count')
        item_counts[identity, ordinal] = item_counts.get((identity, ordinal), 0) + 1
    for key, count in counts.items():
        if item_counts.get(key, 0) != count:
            raise ValueError('IF arm statement coverage is incomplete')
    for identity, (arm_count, has_else) in endings.items():
        sequence = sorted(by_construct.get(identity, []))
        if (has_else not in (0, 1) or len(sequence) != arm_count
                or any(ordinal != index for index, (ordinal, _) in enumerate(sequence, 1))
                or any(role == 'else' for _, role in sequence[:-1])
                or int(bool(sequence and sequence[-1][1] == 'else')) != has_else
                or (modes[identity][2] == 'inline' and arm_count > 2)):
            raise ValueError('IF final arm coverage is incomplete or unordered')
        owner = modes[identity][0]
        for ordinal, _ in sequence:
            suffix = str(identity) + ':' + str(ordinal)
            if not order[owner, 'if-construct:' + str(identity)] < order[owner, 'if-arm:' + suffix] < order[owner, 'if-arm-end:' + suffix] < order[owner, 'if-end:' + str(identity)]:
                raise ValueError('IF arm receipts are outside their complete construct')

    for identity, block in model.constructs.items():
        module = int(model.source_contexts[int(block[6])][4])
        if block[5] == 'if' and model.capabilities[module].get('if-arm-inputs') == 'available' and identity not in modes:
            raise ValueError('Accepted IF construct lacks its original arms')
    for statement, start in model.statements.items():
        work_left -= 1
        identity = int(start[3])
        if work_left < 0:
            raise ValueError('IF accepted statement coverage work budget exceeded')
        if identity not in modes or statement in headers:
            continue
        if modes[identity][2] == 'block' and statement == int(model.construct_endings[identity][2]):
            if statement in members:
                raise ValueError('END IF is classified as a body statement')
            continue
        if model.statement_endings[statement][3] == 'parsed' and statement not in members:
            raise ValueError('Accepted IF body statement lacks its arm')

# end of semantic_if_arms.py
