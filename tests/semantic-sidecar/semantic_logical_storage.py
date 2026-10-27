"""Project: FreeBASIC semantic sidecar
File: semantic_logical_storage.py
Purpose: Validate immutable original logical, comparison and string forests.
Responsibilities: Native EX ownership, target types, closure and capability coverage.
This file intentionally does NOT treat these input snapshots as executable nodes.
"""
import re

from semantic_assignment_storage import COMPLETE_CLASSES, BINARY_CODES, UNARY_CODES, MAX_NODES, MAX_DEPTH, MAX_POINTER_DEPTH
from semantic_datatypes import primitive_name


def validate_logical_storage(model, number, subject_modules, node_kind_count, comparison=False, string_inputs=False):
    storage_key = 'comparison-storage' if comparison else 'logical-storage'
    tree_key = 'comparison-tree:' if comparison else 'logical-tree:'
    capability = 'original-comparison-storage-trees' if comparison else 'original-logical-storage-trees'
    operators = (('equal', 'not-equal', 'less-than', 'less-or-equal', 'greater-than', 'greater-or-equal')
                 if comparison else ('logical-and', 'logical-or'))
    groups, trees = {}, {}
    expressions = {int(row[1]): row for row in model.records['E']}
    if string_inputs:
        storage_key, tree_key = 'string-storage', 'string-tree:'
        capability = 'original-string-storage-trees'
        operators = ('add', 'concatenate')
    work_left = 16000000

    def original_strings(original, module):
        return all(identity in expressions and
                   not int(expressions[identity][12]) & 0x1e0 and
                   primitive_name(model, module, int(expressions[identity][12])) in ("zstring", "wstring", "string", "fixed-string")
                   for identity in map(int, original[4:6]))

    def decimal(text, minimum=0, maximum=9223372036854775807):
        value = number(text, minimum)
        if str(value) != text or value > maximum:
            raise ValueError('Logical storage value is not canonical or exceeds its bound')
        return value

    for row in model.records['K']:
        key = row[3]
        if key != storage_key and not key.startswith(tree_key):
            continue
        work_left -= 1 + len(key) + len(row[4])
        identity = decimal(row[2], 1)
        module = subject_modules.get(('expression', identity))
        original = model.expression_operands.get(identity)
        if (work_left < 0 or row[1] != 'expression' or module is None or original is None
                or original[2] != 'binary' or original[3] not in operators
                or string_inputs and not original_strings(original, module)
                or model.capabilities[module].get(capability) != 'available'):
            raise ValueError('Logical storage has no available native short-circuit owner')
        fields = row[4].split('\t')
        if key == storage_key:
            if len(fields) != 3 or identity in groups:
                raise ValueError('Logical storage root group is repeated or malformed')
            left, right, count = [decimal(field, 1) for field in fields]
            if count < 2 or count > MAX_NODES or left != 1 or not left < right <= count:
                raise ValueError('Logical storage root group exceeds its bounds')
            groups[identity] = module, left, right, count
            continue
        node = decimal(key[len(tree_key):], 1, MAX_NODES)
        if len(key) > 32 or len(fields) != 16 or node in trees.setdefault(identity, {}):
            raise ValueError('Logical storage node is repeated or malformed')
        node_class, dtype, subtype, symbol = [decimal(field) for field in fields[:4]]
        options = decimal(fields[5], 0, 255)
        decimal(fields[6], -9223372036854775808)
        scale = decimal(fields[7], -9223372036854775808)
        flags = [decimal(field, 0, 1) for field in fields[8:11]]
        left, right = decimal(fields[13]), decimal(fields[14])
        if (node_class >= node_kind_count or dtype > 4294967295 or dtype & 31 not in model.primitives[module - 1]
                or (dtype & 0x1e0) >> 5 > MAX_POINTER_DEPTH or any(child and child <= node for child in (left, right))
                or fields[15] not in ('complete', 'opaque')):
            raise ValueError('Logical storage node has invalid types or edges')
        if any(subject and subject_modules.get(('symbol', subject)) != module for subject in (subtype, symbol)):
            raise ValueError('Logical storage contains a missing or foreign symbol')
        if fields[15] == 'opaque':
            if left or right:
                raise ValueError('Opaque logical storage advertises child coverage')
        elif (node_class not in COMPLETE_CLASSES
              or node_class in (3, 9) and (not left or not right)
              or node_class in (4, 5, 6, 19) and (not left or right)
              or node_class in (16, 17, 23) and (left or right)
              or node_class == 18 and not left or node_class == 20 and right):
            raise ValueError('Logical storage claims unsupported node coverage')
        if node_class not in (3, 4, 9) and (fields[4] or options):
            raise ValueError('Logical storage nonoperator advertises an operation')
        if node_class in (3, 4) and (fields[4] or fields[15] == 'complete'):
            if fields[4] not in (BINARY_CODES if node_class == 3 else UNARY_CODES):
                raise ValueError('Logical storage advertises an unknown operator')
        if node_class == 9 and (fields[4] or fields[15] == 'complete'):
            signature = model.signatures.get(symbol)
            if fields[4] != 'power' or options or signature is None or signature[2] != 'function':
                raise ValueError('Logical storage call lacks its selected numeric operation')
        if node_class != 5 and any(flags) or node_class != 18 and scale:
            raise ValueError('Logical storage flags do not match their node class')
        kind, value = fields[11:13]
        if kind in ('signed', 'unsigned'):
            decimal(value, 0 if kind == 'unsigned' else -9223372036854775808,
                    18446744073709551615 if kind == 'unsigned' else 9223372036854775807)
        elif kind == 'float64-bits':
            if re.fullmatch(r'0x[0-9A-F]{16}', value) is None:
                raise ValueError('Logical storage float bits are malformed')
        elif kind != 'none' or value:
            raise ValueError('Logical storage constant kind is invalid')
        if node_class != 16 and kind != 'none' or node_class == 16 and fields[15] == 'complete' and kind == 'none':
            raise ValueError('Logical storage constant does not match its node class')
        trees[identity][node] = fields

    if set(groups) != set(trees):
        raise ValueError('Logical storage lacks its complete forest')
    for identity, (module, left, right, count) in groups.items():
        forest = trees[identity]
        original = model.expression_operands[identity]
        if len(forest) != count or any(node not in forest for node in range(1, count + 1)):
            raise ValueError('Logical storage has incomplete node coverage')
        for root, expression in zip((left, right), map(int, original[4:6])):
            value = expressions.get(expression)
            if value is None or forest[root][:4] != [value[8], value[12], value[14], value[13]]:
                raise ValueError('Logical storage root differs from its original typed expression')
        pending, seen = [(left, 0), (right, 0)], set()
        while pending:
            node, depth = pending.pop()
            work_left -= 1
            if work_left < 0 or node not in forest or node in seen or depth > MAX_DEPTH:
                raise ValueError('Logical storage forest is incomplete, shared or cyclic')
            seen.add(node)
            fields = forest[node]
            if depth == MAX_DEPTH and fields[15] != 'opaque':
                raise ValueError('Logical storage depth coverage is invalid')
            pending.extend((child, depth + 1) for child in map(int, fields[13:15]) if child)
        if len(seen) != count:
            raise ValueError('Logical storage contains an unowned node')
    for identity, original in model.expression_operands.items():
        module = subject_modules['expression', identity]
        if (original[2] == 'binary' and original[3] in operators
                and (not string_inputs or original_strings(original, module))
                and model.capabilities[module].get(capability) == 'available'
                and identity not in groups):
            raise ValueError('Original short-circuit expression lacks its storage trees')


def validate_comparison_storage(model, number, subject_modules, node_kind_count):
    validate_logical_storage(model, number, subject_modules, node_kind_count, comparison=True)

def validate_string_storage(model, number, subject_modules, node_kind_count):
    validate_logical_storage(model, number, subject_modules, node_kind_count, string_inputs=True)

# end of semantic_logical_storage.py
