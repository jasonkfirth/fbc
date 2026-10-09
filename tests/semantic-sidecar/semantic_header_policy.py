"""Project: FreeBASIC semantic sidecar
File: semantic_header_policy.py
Purpose: Validate accepted source occurrence policies and library inputs.
Responsibilities: Capability gates, canonical counts and complete import coverage.
This file intentionally does NOT guess header guards or parse source directives.
"""
FEATURE = 'source-header-policy-inputs'
PREFIX = 'header-library:'


def validate_header_policy(model, number, unescape, subject_modules):
    policies, libraries, imports, identities = {}, {}, {}, set()
    work_left = 8000000
    for row in model.records['K']:
        work_left -= 1
        key = row[3]
        if not key.startswith('header-policy:') and not key.startswith(PREFIX):
            continue
        fields = row[4].split('\t')
        is_policy = key.startswith('header-policy:')
        source_id = number(key[len('header-policy:'):] if is_policy else fields[0], 1)
        source = model.source_contexts.get(source_id)
        owner = model.symbols.get(int(row[2]))
        work_left -= len(key) + len(row[4])
        if (work_left < 0 or row[1] != 'symbol' or source is None or owner is None
                or owner[3] != '8' or owner[11] != '0'
                or subject_modules.get(('symbol', int(row[2]))) != int(source[4])
                or model.capabilities[int(source[4])].get(FEATURE) != 'available'
                or len(fields) != (3 if is_policy else 4)):
            raise ValueError('Invalid source header policy owner or capability')
        if is_policy:
            if key != 'header-policy:' + str(source_id):
                raise ValueError('Noncanonical source header policy key')
            if source_id in policies or fields[0] not in ('0', '1'):
                raise ValueError('Invalid or repeated source header policy')
            counts = [number(value, 0) for value in fields[1:]]
            if list(map(str, counts)) != fields[1:]:
                raise ValueError('Noncanonical source header policy counts')
            policies[source_id] = counts
        else:
            identity = number(key[len(PREFIX):], 1)
            branch_id = number(fields[3], 0)
            if (key != PREFIX + str(identity) or identity in identities
                    or fields[0] != str(source_id) or fields[1] not in ('inclib', 'libpath') or '\0' in unescape(fields[2])
                    or str(branch_id) != fields[3]):
                raise ValueError('Invalid source header library input')
            if branch_id:
                branch = model.conditional_branches.get(branch_id)
                if (branch is None or model.source_contexts[int(branch[4])][4] != source[4]):
                    raise ValueError('Foreign source header library conditional')
            identities.add(identity)
            libraries[source_id] = libraries.get(source_id, 0) + 1
    for row in model.records['SOP']:
        work_left -= 1
        if work_left < 0:
            raise ValueError('Source header policy work budget exceeded')
        if row[2] == 'namespace-import':
            source_id = int(model.statements[int(row[1])][5])
            imports[source_id] = imports.get(source_id, 0) + 1
    expected = {source_id for source_id, row in model.source_contexts.items()
                if model.capabilities[int(row[4])].get(FEATURE) == 'available'}
    if set(policies) != expected:
        raise ValueError('Source header policy occurrence coverage is incomplete')
    for source_id, counts in policies.items():
        if counts != [libraries.get(source_id, 0), imports.get(source_id, 0)]:
            raise ValueError('Source header policy input counts disagree')
# end of semantic_header_policy.py
