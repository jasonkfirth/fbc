"""Project: FreeBASIC semantic sidecar tests
File: source_memory_inputs.py
Purpose: Preserve selected user New/Delete applications and declaration contexts.
Responsibilities: Original occurrences, macro anchors, export modes and damage.
This file intentionally does NOT execute allocations or claim recursion coverage.
"""
from collections import Counter
import subprocess

from sidecar import Model
from semantic_source_memory import FEATURE, PREFIX, CLOSURE

BODY = '''Type Payload
    Declare Operator New(ByVal bytes As UInteger) As Any Ptr
    Declare Operator New[](ByVal bytes As UInteger) As Any Ptr
    Declare Operator Delete(ByVal storage As Any Ptr)
    Declare Operator Delete[](ByVal storage As Any Ptr)
    value As Integer
End Type
Operator Payload.New(ByVal bytes As UInteger) As Any Ptr
    Return Allocate(bytes)
End Operator
Operator Payload.New[](ByVal bytes As UInteger) As Any Ptr
    Return Allocate(bytes)
End Operator
Operator Payload.Delete(ByVal storage As Any Ptr)
    Deallocate(storage)
End Operator
Operator Payload.Delete[](ByVal storage As Any Ptr)
    Deallocate(storage)
End Operator
Declare Sub DefaultUser(ByVal item As Payload Ptr = New Payload)
Sub DefaultUser(ByVal item As Payload Ptr)
End Sub
Type Holder
    item As Payload Ptr = New Payload
End Type
#Define MAKE_PAYLOAD() New Payload
#Define WRAPPED_PAYLOAD() MAKE_PAYLOAD()
Sub SourceApplications()
    Dim item As Payload Ptr = New Payload
    Dim items As Payload Ptr = New Payload[2]
    Delete item
    Delete[] items
    Dim ignored_size As Integer = SizeOf(New Payload)
    Dim macro_item As Payload Ptr = MAKE_PAYLOAD()
    Dim nested_item As Payload Ptr = WRAPPED_PAYLOAD()
    Dim built_in As Integer Ptr = New Integer
    Delete built_in
    Dim storage(0 To 255) As UByte
    Dim placed_item As Payload Ptr = New (@storage(0)) Payload
    Dim owner_value As Holder
#Line 700 "logical-memory-only.bas"
#Include "memory-child.bi"
    Dim remapped_item As Payload Ptr = WRAPPED_PAYLOAD()
End Sub
'''


def fixture(test):
    from assignment_inputs import source_text
    test.source("' Project: source memory facts\n' File: memory-child.bi\n"
                "' Purpose: Preserve allocation origins in an included procedure body.\n"
                "Dim included_item As Payload Ptr = MAKE_PAYLOAD()\n"
                "' end of memory-child.bi\n", 'memory-child.bi')
    return test.source(source_text(BODY, 'source-memory.bas'), 'source-memory.bas')


def applications(model):
    return [row for row in model.records['K'] if row[3].startswith(PREFIX)]


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            output = test.emission_path(source.stem, backend)
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            _, path = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(path)
            model.validate_physical_locations()
            test.assertEqual(model.capabilities[1][FEATURE], 'available')
            rows = applications(model)
            test.assertEqual(len(rows), 11)
            test.assertEqual(Counter(row[4].split('\t')[0] for row in rows),
                             Counter({'allocate': 8, 'allocate-array': 1, 'deallocate': 1, 'deallocate-array': 1}))
            test.assertEqual(Counter(row[4].split('\t')[4] for row in rows),
                             Counter({'direct': 9, 'formal-default': 1, 'field-initializer': 1}))
            macro_rows = [row for row in rows if row[4].split('\t')[3] != '0']
            test.assertEqual(len(macro_rows), 4)
            for row in macro_rows:
                source_id = row[4].split('\t')[2]
                location = model.physical_locations['source-context', int(source_id), row[3]]
                test.assertEqual(location[11], 'mapped')
                test.assertNotEqual(int(location[5]), 700)
                operation = model.rows[model.rows.index(row) - 1]
                test.assertEqual(operation[0], 'O')
                test.assertEqual(operation[4], '0', 'Invocation anchors are not editable operator tokens')
            test.assertEqual(output.read_bytes(), plain)
            binary = test.working / ('source-memory-' + backend + '.fbcsem')
            reply = subprocess.run([str(test.compiler), '-prefix', str(test.toolchain_prefix), '-i', str(test.root / 'inc'),
                                    '-gen', backend, '-r', '-maxerr', '20', '-semantic-model', str(binary), *extra, str(source)],
                                   cwd=test.working, capture_output=True, text=True, timeout=120)
            test.assertEqual(reply.returncode, 0, reply.stdout + reply.stderr)
            test.assertTrue(Model.read(binary).rows == model.rows, 'Binary transport changed source memory facts')
            test.assertEqual(output.read_bytes(), plain)
            for mode in ('bindings', 'expressions', 'macro-compact'):
                if mode == 'macro-compact':
                    _, compact_path = test.invoke([source], backend=backend, extra=(*extra, '-semantic-model-compact'))
                    compact = Model.read(compact_path)
                else:
                    compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(applications(compact))
                test.assertFalse(any(row[3] == CLOSURE for row in compact.records['K']))
                test.assertEqual(output.read_bytes(), plain)
            empty = test.source("' Empty source-memory observation module\n", 'empty-memory.bas')
            empty_model = test.compile(empty, backend=backend)
            test.assertEqual([row[4] for row in empty_model.records['K'] if row[3] == CLOSURE], ['0'])


MUTATIONS = ('count', 'missing-count', 'missing-application', 'duplicate', 'role', 'statement',
             'source', 'macro', 'context', 'target', 'missing-target', 'missing-context',
             'missing-location', 'missing-origin', 'unavailable')


def damaged_rows(rows, mutation):
    changed = [row[:] for row in rows]
    app = next(row for row in changed if row[0] == 'K' and row[3].startswith(PREFIX))
    fields = app[4].split('%09')
    count = next(row for row in changed if row[0] == 'K' and row[3] == CLOSURE)
    if mutation == 'count': count[4] = str(int(count[4]) + 1)
    elif mutation == 'missing-count': changed.remove(count)
    elif mutation == 'missing-application': changed.remove(app)
    elif mutation == 'duplicate': changed.insert(changed.index(app) + 1, app[:])
    elif mutation in ('role', 'statement', 'source', 'macro', 'context'):
        slot = {'role': 0, 'statement': 1, 'source': 2, 'macro': 3, 'context': 4}[mutation]
        fields[slot] = 'unknown' if mutation in ('role', 'context') else '4294967295'
        app[4] = '%09'.join(fields)
    elif mutation in ('target', 'missing-target'):
        target = next(row for row in changed if row[0] == 'H' and row[5] == 'source-memory-target' and row[6] == '1')
        if mutation == 'target': target[4] = app[2]
        else: changed.remove(target)
    elif mutation == 'missing-context':
        context = next(row for row in changed if row[0] == 'H' and row[5] == 'source-memory-default-formal')
        changed.remove(context)
    elif mutation == 'missing-location':
        location = next(row for row in changed if row[0] == 'LOC' and row[3] == app[3])
        changed.remove(location)
    elif mutation == 'missing-origin':
        origin = next(row for row in changed if row[0] == 'MR' and row[4].startswith(PREFIX))
        changed.remove(origin)
    elif mutation == 'unavailable': next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    else: raise ValueError('Unknown source memory mutation')
    return changed


def check_rejection(test):
    from parsed_expression_coverage import write_rows
    for backend in test.backends:
        source = fixture(test)
        _, artifact = test.invoke([source], backend=backend)
        Model.read(artifact)
        rows = [line.split('\t') for line in artifact.read_text(encoding='ascii').splitlines()]
        damaged = test.working / 'damaged-source-memory.tsv'
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(damaged, damaged_rows(rows, mutation))
                with test.assertRaises(ValueError): Model.read(damaged)

# end of source_memory_inputs.py
