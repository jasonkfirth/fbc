"""Project: FreeBASIC semantic sidecar
File: semantic_assignment_storage.py
Purpose: Validate immutable original assignment value and storage trees.
Responsibilities: Typed roots, complete forests, markers and destination origins.
This file intentionally does NOT classify these snapshots as executable nodes.
"""
import re

PREFIXES = ('assignment-storage:', 'assignment-tree:')
COMPLETE_CLASSES = frozenset((3, 4, 5, 6, 9, 16, 17, 18, 19, 20, 23))
MAX_NODES = 65536
MAX_DEPTH = 64
BINARY_CODES = frozenset(('add', 'subtract', 'multiply', 'divide', 'integer-divide',
    'modulo', 'and', 'or', 'logical-and', 'logical-or', 'xor', 'equivalence',
    'implication', 'shift-left', 'shift-right', 'power', 'concatenate', 'equal',
    'greater-than', 'less-than', 'not-equal', 'greater-or-equal', 'less-or-equal',
    'identity-test', 'arctangent2'))
UNARY_CODES = frozenset(('negate', 'unary-plus', 'not', 'logical-not', 'length',
    'absolute-value', 'sign', 'sine', 'arcsine', 'cosine', 'arccosine', 'tangent',
    'arctangent', 'square-root', 'reciprocal-square-root', 'reciprocal', 'logarithm',
    'exponential', 'floor', 'truncate', 'fractional-part'))


def validate_assignment_storage(model, number, subject_modules, node_kind_count):
    groups, trees, properties, markers, origins = {}, {}, {}, set(), set()
    inputs = {}
    expressions = {int(row[1]): row for row in model.records['E']}
    work_left = 16000000

    def decimal(text, minimum=0, maximum=9223372036854775807):
        value = number(text, minimum)
        if str(value) != text or value > maximum:
            raise ValueError('Assignment storage identity is not canonical')
        return value

    for row in model.records['K']:
        if row[3].startswith('assignment-input:'):
            _, statement, ordinal = row[3].split(':')
            inputs[int(statement), int(ordinal)] = row
        if not row[3].startswith(PREFIXES):
            continue
        work_left -= 1 + len(row[3]) + len(row[4])
        owner = decimal(row[2], 1)
        module = subject_modules.get(('symbol', owner))
        parts = row[3].split(':')
        if (work_left < 0 or row[1] != 'symbol' or module is None
                or model.capabilities[module].get('assignment-storage-trees') != 'available'
                or len(parts) != (3 if parts[0] == 'assignment-storage' else 4)
                or (owner, row[3]) in properties):
            raise ValueError('Assignment storage property is malformed, repeated or unavailable')
        statement, ordinal = decimal(parts[1], 1), decimal(parts[2], 1)
        start = model.statements.get(statement)
        if start is None or int(start[4]) != owner or int(start[7]) != module:
            raise ValueError('Assignment storage belongs to another procedure or statement')
        key = statement, ordinal
        properties[owner, row[3]] = row
        fields = row[4].split('\t')
        if parts[0] == 'assignment-storage':
            if len(fields) != 3 or key in groups:
                raise ValueError('Assignment storage root group is repeated or malformed')
            left, right, count = [decimal(field, 1) for field in fields]
            if count < 2 or count > MAX_NODES or left != 1 or not left < right <= count:
                raise ValueError('Assignment storage root group exceeds its bounds')
            groups[key] = owner, module, left, right, count
        else:
            identity = decimal(parts[3], 1)
            if len(fields) != 16 or identity > MAX_NODES or identity in trees.setdefault(key, {}):
                raise ValueError('Assignment storage node is repeated or malformed')
            node_class, dtype, subtype, symbol = [decimal(field) for field in fields[:4]]
            options = decimal(fields[5])
            decimal(fields[6], -9223372036854775808)
            scale = decimal(fields[7], -9223372036854775808)
            flags = [decimal(field) for field in fields[8:11]]
            left, right = decimal(fields[13]), decimal(fields[14])
            if (node_class >= node_kind_count or dtype > 4294967295 or dtype & 31 not in model.primitives[module - 1]
                    or (dtype & 0x1e0) >> 5 > 8 or options > 255 or any(flag > 1 for flag in flags)
                    or any(child and child <= identity for child in (left, right))
                    or fields[15] not in ('complete', 'opaque')):
                raise ValueError('Assignment storage node has invalid types, flags or edges')
            for subject in (subtype, symbol):
                if subject and subject_modules.get(('symbol', subject)) != module:
                    raise ValueError('Assignment storage node has a missing or foreign symbol')
            if fields[15] == 'opaque':
                if left or right:
                    raise ValueError('Opaque assignment storage advertises child coverage')
            else:
                if node_class not in COMPLETE_CLASSES:
                    raise ValueError('Assignment storage claims an unsupported complete node')
                if node_class in (3, 9) and (not left or not right):
                    raise ValueError('Assignment binary value lacks its operands')
                if node_class in (4, 5, 6, 19) and (not left or right):
                    raise ValueError('Assignment unary storage/value has an invalid edge')
                if node_class in (16, 17, 23) and (left or right):
                    raise ValueError('Assignment storage atom has an unexpected child')
                if node_class == 18 and not left:
                    raise ValueError('Assignment array index lacks its value input')
                if node_class == 20 and right:
                    raise ValueError('Assignment dereference has an unexpected right edge')
            if node_class not in (3, 4, 9) and (fields[4] or options):
                raise ValueError('Assignment storage nonoperator advertises an operation')
            if node_class in (3, 4):
                codes = BINARY_CODES if node_class == 3 else UNARY_CODES
                if fields[4] not in codes and (fields[4] or fields[15] == 'complete'):
                    raise ValueError('Assignment storage operator has an unknown selected code')
            if node_class == 9 and (fields[4] or fields[15] == 'complete'):
                signature = model.signatures.get(symbol)
                if fields[4] != 'power' or options or signature is None or signature[2] != 'function':
                    raise ValueError('Assignment storage call lacks its selected numeric operation')
            if node_class != 5 and any(flags):
                raise ValueError('Assignment storage nonconversion advertises conversion flags')
            if node_class != 18 and scale:
                raise ValueError('Assignment storage nonindex advertises a scale')
            kind, value = fields[11:13]
            if kind in ('signed', 'unsigned'):
                decimal(value, 0 if kind == 'unsigned' else -9223372036854775808,
                        18446744073709551615 if kind == 'unsigned' else 9223372036854775807)
            elif kind == 'float64-bits':
                if re.fullmatch(r'0x[0-9A-F]{16}', value) is None:
                    raise ValueError('Assignment storage float bits are malformed')
            elif kind != 'none' or value:
                raise ValueError('Assignment storage constant kind is invalid')
            if (node_class != 16 and kind != 'none') or (node_class == 16 and fields[15] == 'complete' and kind == 'none'):
                raise ValueError('Assignment storage constant does not match its node class')
            trees[key][identity] = fields

    for row in model.records['H']:
        work_left -= 1 + len(row[5])
        if work_left < 0:
            raise ValueError('Assignment storage marker work budget exceeded')
        if not row[5].startswith(PREFIXES):
            continue
        kind, suffix = row[5].split(':', 1)
        key = kind + ':' + row[6] + ':' + suffix
        owner = decimal(row[2], 1)
        if (row[1] != 'symbol' or row[3] != 'symbol' or row[4] != row[2]
                or (owner, key) not in properties or (owner, key) in markers):
            raise ValueError('Assignment storage marker is repeated or lacks its property')
        markers.add((owner, key))
    if set(properties) != markers:
        raise ValueError('Assignment storage property lacks its independent marker')
    for tag, role_field in (('LOC', 3), ('MR', 4)):
        for row in model.records[tag]:
            work_left -= 1 + len(row[role_field])
            if work_left < 0:
                raise ValueError('Assignment destination origin work budget exceeded')
            if not row[role_field].startswith('assignment-destination:'):
                continue
            statement, ordinal = decimal(row[2], 1), decimal(row[role_field][23:], 1)
            key = statement, ordinal
            if row[1] != 'statement' or key not in groups:
                raise ValueError('Assignment destination origin lacks its storage group')
            source = (int(row[4]) if tag == 'LOC'
                      else int(model.macro_invocations[int(row[3])][4]))
            if int(model.source_contexts[source][4]) != groups[key][1]:
                raise ValueError('Assignment destination origin belongs to another module')
            origins.add(key)
    if set(groups) != set(trees) or set(groups) != origins:
        raise ValueError('Assignment storage lacks its forest or destination origin')
    for key, (owner, module, left, right, count) in groups.items():
        entry = inputs.get(key)
        forest = trees[key]
        if entry is None or len(forest) != count or any(index not in forest for index in range(1, count + 1)):
            raise ValueError('Assignment storage lacks complete accepted input coverage')
        expression_ids = [int(field) for field in entry[4].split('\t')[:2]]
        for root, expression in zip((left, right), expression_ids):
            value = expressions[expression]
            if forest[root][:4] != [value[8], value[12], value[14], value[13]]:
                raise ValueError('Assignment storage root differs from its original typed expression')
        parents = {left: 0, right: 0}
        pending = [(left, 0), (right, 0)]
        seen = set()
        while pending:
            identity, depth = pending.pop()
            work_left -= 1
            if work_left < 0 or identity not in forest or identity in seen or depth > MAX_DEPTH:
                raise ValueError('Assignment storage forest is incomplete or cyclic')
            seen.add(identity)
            fields = forest[identity]
            if depth == MAX_DEPTH and fields[15] != 'opaque':
                raise ValueError('Assignment storage depth coverage is invalid')
            for child in map(int, fields[13:15]):
                if not child:
                    continue
                if child in parents:
                    raise ValueError('Assignment storage node belongs to multiple parents')
                parents[child] = identity
                pending.append((child, depth + 1))
        if len(seen) != count:
            raise ValueError('Assignment storage contains an unowned node')
    for key, entry in inputs.items():
        module = subject_modules['symbol', int(entry[2])]
        if model.capabilities[module].get('assignment-storage-trees') == 'available' and key not in groups:
            raise ValueError('Accepted assignment lacks its original storage trees')

# end of semantic_assignment_storage.py
