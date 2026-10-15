"""Project: FreeBASIC semantic sidecar tests
File: virtual_call_inputs.py
Purpose: Keep selected virtual procedures complete without source bindings.
Responsibilities: Genuine dispatch facts, missing targets and contradictory roles.
This file intentionally does NOT infer dynamic overrides or execute methods.
"""
from sidecar import Model
from parsed_expression_coverage import write_rows

BODY = '''Type Device Extends Object
    value As Integer
    Declare Virtual Sub Apply(ByVal inputValue As Integer)
    Declare Virtual Operator Let(ByRef other As Device)
End Type
Sub Device.Apply(ByVal inputValue As Integer)
    value = inputValue
End Sub
Operator Device.Let(ByRef other As Device)
    value = other.value
End Operator
Sub Observe(ByVal inputValue As Integer)
End Sub
Sub Dispatch(ByRef destination As Device, ByRef sourceValue As Device)
    destination = sourceValue
    destination.Apply(1)
    Observe(2)
End Sub
'''
MUTATIONS = ('missing-unbound-target', 'duplicate-target', 'nonprocedure-target',
             'noncall-owner', 'nonvirtual-owner', 'direct-call-target', 'missing-bound-target')


def fixture_text():
    return ("' Project: FreeBASIC semantic sidecar tests\n"
            "' File: virtual-call-inputs.bas\n"
            "' Purpose: Exercise native virtual target ownership.\n"
            "' Responsibilities: Bound calls, operator calls and direct controls.\n"
            "' This file intentionally does NOT execute the dispatch.\n" + BODY
            + "' end of virtual-call-inputs.bas\n")


def control_rows(rows, model, mutation):
    changed = [row[:] for row in rows]
    owner, = model.named('Dispatch', 'procedure')
    calls = [row for identity, row in model.nodes.items() if row[4] == '9'
             and model.statement_owners.get(('node', identity)) is not None
             and model.statements[model.statement_owners['node', identity]][4] == str(owner)]
    virtual = [row for row in calls if model.properties['node', int(row[1])]['call-kind'] == 'virtual']
    assert len(virtual) == 2
    bound = {row[2] for row in model.records['H']
             if row[1] == 'node' and row[3] == 'binding' and row[5] == 'source-binding'}
    unbound, = [row for row in virtual if row[1] not in bound]
    written, = [row for row in virtual if row[1] in bound]
    selected = next(row for row in changed if row[:4] == ['H', 'node', unbound[1], 'symbol']
                    and row[5] == 'static-target')
    if mutation == 'missing-unbound-target':
        changed.remove(selected)
    elif mutation == 'duplicate-target':
        changed.insert(changed.index(selected), selected[:])
    elif mutation == 'nonprocedure-target':
        selected[4] = str(model.named('Device', 'type')[0])
    elif mutation == 'noncall-owner':
        selected[2] = next(row[1] for row in model.records['N'] if row[4] == '17')
    elif mutation == 'nonvirtual-owner':
        kind = next(row for row in changed if row[:4] == ['K', 'node', unbound[1], 'call-kind'])
        kind[4] = 'indirect'
    elif mutation == 'direct-call-target':
        direct, = [row for row in calls if model.properties['node', int(row[1])]['call-kind'] == 'direct']
        selected[2] = direct[1]
        # Retain the genuine virtual selection so this control isolates the
        # extra relation on the otherwise valid direct CALL.
        original = next(row for row in rows if row[:4] == ['H', 'node', unbound[1], 'symbol']
                        and row[5] == 'static-target')
        changed.insert(changed.index(selected), original[:])
    elif mutation == 'missing-bound-target':
        changed.remove(next(row for row in changed if row[:4] == ['H', 'node', written[1], 'symbol']
                            and row[5] == 'static-target'))
    else:
        raise ValueError('Unknown virtual target control: ' + mutation)
    return changed


def check_rejection(test):
    for backend in test.backends:
        source = test.source(fixture_text(), 'virtual-call-inputs.bas')
        _, path = test.invoke([source], backend=backend)
        model = Model.read(path)
        model.validate_physical_locations()
        rows = [line.split('\t') for line in path.read_text().splitlines()]
        damaged = test.working / 'damaged-virtual-target.tsv'
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(damaged, control_rows(rows, model, mutation))
                with test.assertRaises(ValueError):
                    Model.read(damaged)

# end of virtual_call_inputs.py
