"""Project: FreeBASIC semantic sidecar
File: semantic_abi_policy.py
Purpose: Validate original managed-string and convention header choices.
Responsibilities: Header pairing, canonical flags and occurrence closure.
This file intentionally does NOT infer ABI choices from signature defaults.
"""
FEATURE = 'procedure-abi-policy-inputs'
PREFIX = 'abi-policy-input:'


def validate_abi_policy_inputs(model, number, subject_modules):
    headers, policies, markers = {}, {}, set()
    work_left = 8000000
    for row in model.records['K']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('ABI policy work budget exceeded')
        if row[3].startswith('procedure-abi-input:'):
            identity = number(row[3].split(':')[1], 1)
            headers[int(row[2]), identity] = row
        elif row[3].startswith(PREFIX):
            suffix = row[3][len(PREFIX):]
            identity, owner = number(suffix, 1), number(row[2], 1)
            module = subject_modules.get(('symbol', owner))
            work_left -= len(row[3]) + len(row[4])
            fields = row[4].split('\t')
            if (work_left < 0 or row[1] != 'symbol' or str(identity) != suffix
                    or fields not in (['0', '0'], ['0', '1'], ['1', '0'], ['1', '1'])
                    or model.capabilities.get(module, {}).get(FEATURE) != 'available'
                    or (owner, identity) in policies):
                raise ValueError('ABI policy flags, identity or capability are invalid')
            policies[owner, identity] = row
    expected = {key for key in headers
                if model.capabilities[subject_modules['symbol', key[0]]].get(FEATURE) == 'available'}
    if set(policies) != expected:
        raise ValueError('ABI policy header coverage is incomplete')
    for row in model.records['H']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('ABI policy marker work budget exceeded')
        if not row[5].startswith(PREFIX):
            continue
        suffix = row[5][len(PREFIX):]
        identity, owner = number(suffix, 1), number(row[2], 1)
        key = owner, identity
        header = headers.get(key)
        if (row[1] != 'symbol' or row[3] != 'symbol' or row[4] != row[2]
                or str(identity) != suffix or key not in policies or key in markers
                or header is None or row[6] != header[4].split('\t')[0]):
            raise ValueError('ABI policy marker lacks its original header')
        markers.add(key)
    if markers != set(policies):
        raise ValueError('ABI policy lacks its occurrence marker')
# end of semantic_abi_policy.py
