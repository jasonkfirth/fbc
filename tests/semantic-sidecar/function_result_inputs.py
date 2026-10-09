"""Project: FreeBASIC semantic sidecar tests
File: function_result_inputs.py
Purpose: Preserve accepted numeric function-result assignments before conversion.
Responsibilities: Original values, destination types and BYREF exclusions.
This file intentionally does NOT infer function result types from source text.
"""
from sidecar import source_range

BODY = '''Enum State
    Ready = 3
    Busy = 6
End Enum
Dim Shared result_flag As Boolean
Function boolean_return(ByVal enum_input As State) As Boolean
    Return enum_input ' boolean-return
End Function
Function boolean_assign(ByVal enum_input As State) As Boolean
    boolean_assign = enum_input ' boolean-assign
End Function
Function byte_return(ByVal wide_input As LongInt) As Byte
    Return wide_input ' byte-return
End Function
Function integer_return(ByVal wide_input As LongInt) As Integer
    Return wide_input ' integer-return
End Function
Function wide_return(ByVal integer_input As Integer) As LongInt
    Return integer_input ' wide-return
End Function
Function single_return(ByVal double_input As Double) As Single
    Return double_input ' single-return
End Function
Function double_return(ByVal single_input As Single) As Double
    Return single_input ' double-return
End Function
Function constant_return() As Boolean
    Return State.Ready ' constant-return
End Function
Function enum_return(ByVal enum_input As State) As State
    Return enum_input ' enum-return
End Function
Function pointer_return(ByVal pointer_input As Integer Ptr) As Integer Ptr
    Return pointer_input ' pointer-return
End Function
Function string_return(ByRef string_input As String) As String
    Return string_input ' string-return
End Function
Function reference_return() ByRef As Boolean
    Return result_flag ' reference-return
End Function
Function address_return() ByRef As Boolean
    Return ByVal @result_flag ' address-return
End Function
Type PlainResult
    value As Integer
End Type
Function aggregate_return(ByRef record_input As PlainResult) As PlainResult
    Return record_input ' aggregate-return
End Function
'''
EXPECTED = {
    'boolean-return': (1, 10), 'boolean-assign': (1, 10),
    'byte-return': (2, 13), 'integer-return': (8, 13),
    'wide-return': (13, 8), 'single-return': (15, 16),
    'double-return': (16, 15), 'constant-return': (1, 10),
}


def check_results(test):
    filename = 'function-result-inputs.bas'
    text = ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + '\n'
            "' Purpose: Exercise original function-result values.\n"
            "' Responsibilities: Numeric conversions and reference controls.\n"
            "' This file intentionally does NOT execute narrowed values.\n"
            '#Lang "fb"\n' + BODY + "' end of " + filename + '\n')
    lines = {line.split("' ", 1)[1]: number for number, line in enumerate(text.splitlines(), 1)
             if "' " in line and not line.startswith("'")}
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.source(text, filename)
            emitted = test.emission_path('result', backend)
            extra = ('-o', str(emitted))
            plain_result, _ = test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            full_result, path = test.invoke([source], backend=backend, extra=extra)
            from sidecar import Model
            model = Model.read(path)
            test.assertTrue(all(features['numeric-function-result-inputs'] == 'available'
                                for features in model.capabilities.values()))
            test.assertEqual(emitted.read_bytes(), plain)
            test.assertEqual(full_result.stdout + full_result.stderr,
                             plain_result.stdout + plain_result.stderr)
            expressions = {int(row[1]): row for row in model.records['E']}
            receipts = [(identity, properties) for (domain, identity), properties in model.properties.items()
                        if domain == 'expression' and 'assignment-target-dtype' in properties]
            test.assertEqual(len(receipts), len(EXPECTED))
            actual = {}
            for identity, properties in receipts:
                row = expressions[identity]
                line = source_range(row, 3)[1]
                label = next(label for label, number in lines.items() if number == line)
                test.assertEqual(properties['assignment-kind'], 'assignment')
                test.assertNotIn('source-assignment-symbol', properties)
                actual[label] = (int(properties['assignment-target-dtype']), int(row[12]))
            test.assertEqual(actual, EXPECTED)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), plain)
                test.assertFalse(any('assignment-target-dtype' in properties
                                     for properties in compact.properties.values()))
                test.assertTrue(all(features['numeric-assignment-targets'] == 'unavailable'
                                    for features in compact.capabilities.values()))
                test.assertTrue(all(features['numeric-function-result-inputs'] == 'unavailable'
                                    for features in compact.capabilities.values()))

# end of function_result_inputs.py
