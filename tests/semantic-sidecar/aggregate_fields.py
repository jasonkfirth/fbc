"""Project: FreeBASIC semantic sidecar tests
File: aggregate_fields.py
Purpose: Validate explicit finalized aggregate source-field order.
Responsibilities: Complete native lists, ownership, unique fields and bounds.
This file intentionally does NOT infer declaration order from storage offsets.
"""


def validate_aggregate_fields(model, number, subject_modules):
    owners, counts = {}, {}

    def available(module):
        return model.capabilities[module].get('aggregate-field-order') == 'available'

    for (domain, identity), properties in model.properties.items():
        entries = [(key, value) for key, value in properties.items() if key.startswith('declared-field:')]
        if not entries:
            continue
        module = subject_modules.get((domain, identity))
        symbol = model.symbols.get(identity)
        if domain != 'symbol' or not available(module) or symbol is None or symbol[3] != '10' or properties.get('layout-finalized') != '1':
            raise ValueError('Field order lacks its finalized aggregate owner')
        expected = number(properties.get('declared-field-count', ''), 0)
        if expected > len(model.records['T']) or len(entries) != expected:
            raise ValueError('Field order count is incomplete or oversized')
        ordinals = set()
        for key, value in entries:
            ordinal, field = number(key[len('declared-field:'):], 1), number(value, 1)
            detail = model.types.get(field)
            if key[len('declared-field:'):] != str(ordinal) or ordinal > expected or ordinal in ordinals or field in owners or detail is None or detail[3] != 'field' or detail[18] != 'source' or int(detail[8]) != identity or subject_modules.get(('symbol', field)) != module:
                raise ValueError('Field order has an invalid ordinal, member or owner')
            ordinals.add(ordinal)
            owners[field] = identity
        counts[identity] = len(entries)
    for identity, detail in model.types.items():
        if not available(subject_modules['symbol', identity]):
            continue
        properties = model.properties['symbol', identity]
        if model.symbols[identity][3] == '10' and properties.get('layout-finalized') == '1':
            expected = number(properties.get('declared-field-count', ''), 0)
            if expected > len(model.records['T']) or counts.get(identity, 0) != expected:
                raise ValueError('Finalized aggregate lacks its complete ordered fields')
        elif detail[3] == 'field' and detail[18] == 'source':
            owner = int(detail[8])
            if model.properties['symbol', owner].get('layout-finalized') == '1' and owners.get(identity) != owner:
                raise ValueError('Source field is absent from its finalized aggregate')

# end of aggregate_fields.py
