"""Project: FreeBASIC semantic sidecar tests
File: semantic_member_selections.py
Purpose: Validate parser-selected member reference roles and receiver identities.
Responsibilities: Per-occurrence coverage, macro associations and module counts.
This file intentionally does NOT classify member syntax from source text.
"""

FEATURE = 'selected-member-roles'
REFERENCE = 'selected-member-reference-'
CLOSURE = 'selected-member-count:'
BINDING = 'selected-member-binding-'
LIMIT = 1000000


def validate_member_selections(model, number, subject_modules):
    bindings = model.records['B']
    counts, closures, references, observed_bindings = {}, {}, {}, set()
    work_left = len(model.rows) * 4 + 8000000

    def decimal(text, minimum=0):
        value = number(text, minimum)
        if str(value) != text or value > 4294967295:
            raise ValueError('Selected member identity is not canonical or bounded')
        return value

    def available(module):
        return model.capabilities.get(module, {}).get(FEATURE) == 'available'

    def selected(field, role, receiver, module):
        symbol = model.symbols.get(field)
        if (symbol is None or symbol[3] != '12'
                or subject_modules.get(('symbol', field)) != module
                or role not in ('expression', 'type', 'symbol')):
            raise ValueError('Selected member has an invalid field or role')
        if role == 'symbol':
            if receiver:
                raise ValueError('Symbol-only field reference has an object receiver')
        elif (not receiver or model.symbols.get(receiver, ['', '', '', ''])[3] != '10'
              or subject_modules.get(('symbol', receiver)) != module):
            raise ValueError('Selected member receiver is not a same-module aggregate')
        counts[module] = counts.get(module, 0) + 1
        if counts[module] > LIMIT:
            raise ValueError('Selected member count exceeds its bound')

    for (domain, identity), properties in model.properties.items():
        keys = [key for key in properties if key.startswith('selected-member-')]
        if not keys:
            continue
        module = subject_modules.get((domain, identity))
        if not available(module):
            raise ValueError('Selected member metadata lacks its capability')
        for key in keys:
            work_left -= 1
            if work_left < 0 or len(key) > 64 or len(properties[key]) > 128:
                raise ValueError('Selected member validation work limit exceeded')
            if domain != 'symbol':
                raise ValueError('Selected member observation has an invalid owner')
            if key.startswith(BINDING):
                ordinal = decimal(key[len(BINDING):], 1)
                payload = properties[key].split('\t')
                if (len(payload) != 2 or not 1 <= ordinal <= len(bindings)
                        or ordinal in observed_bindings):
                    raise ValueError('Selected member binding group has an invalid shape or identity')
                binding = bindings[ordinal - 1]
                if (binding[2] != 'reference' or binding[1] != str(identity)
                        or subject_modules.get(('binding', ordinal)) != module):
                    raise ValueError('Selected member binding group has another field or module')
                selected(identity, payload[0], decimal(payload[1]), module)
                observed_bindings.add(ordinal)
                continue
            if key.startswith(CLOSURE):
                if (decimal(key[len(CLOSURE):], 1) != module or module in closures
                        or model.symbols[identity][3] != '8' or model.symbols[identity][11] != '0'):
                    raise ValueError('Selected member module count has an invalid owner')
                closures[module] = decimal(properties[key])
                if closures[module] > LIMIT:
                    raise ValueError('Selected member module count exceeds its bound')
                continue
            if not key.startswith(REFERENCE):
                raise ValueError('Unknown selected member property')
            reference = decimal(key[len(REFERENCE):], 1)
            payload = properties[key].split('\t')
            if len(payload) != 3 or reference in references:
                raise ValueError('Selected member macro group has an invalid shape or identity')
            role, receiver, expansion = payload[0], decimal(payload[1]), decimal(payload[2], 1)
            invocation = model.macro_invocations.get(expansion)
            origin = model.macro_origins.get(('symbol', identity, 'reference-' + str(reference)))
            if (invocation is None or origin != expansion
                    or model.source_contexts[int(invocation[4])][4] != str(module)):
                raise ValueError('Selected member macro group lacks its exact reference origin')
            selected(identity, role, receiver, module)
            references[reference] = (identity, expansion)

    for ordinal, binding in enumerate(bindings, 1):
        field = decimal(binding[1], 1)
        module = subject_modules.get(('binding', ordinal))
        if available(module) and binding[2] == 'reference' and model.symbols[field][3] == '12':
            if ordinal not in observed_bindings:
                raise ValueError('Selected field binding lacks its complete role group')
    for (domain, field, role), expansion in model.macro_origins.items():
        if domain != 'symbol' or not role.startswith('reference-') or model.symbols[field][3] != '12':
            continue
        module = subject_modules.get(('symbol', field))
        if available(module) and references.get(decimal(role[len('reference-'):], 1)) != (field, expansion):
            raise ValueError('Selected field macro reference lacks its role group')
    for module in model.capabilities:
        if available(module) and closures.get(module) != counts.get(module, 0):
            raise ValueError('Selected member observation coverage is incomplete')

# end of semantic_member_selections.py
