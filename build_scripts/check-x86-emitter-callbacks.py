"""Project: FreeBASIC compiler structural checks
File: check-x86-emitter-callbacks.py
Purpose: Guard the typed x86 opcode callback contract.
Responsibilities: Check table coverage, callback kinds, SSE overrides and
dispatch members without building or installing the compiler.
This file intentionally does NOT replace assembly or native backend tests.
"""
import argparse
from pathlib import Path
import re


def check(source):
    files = {name: (source / name).read_text(encoding='utf-8')
             for name in ('emit.bi', 'emit-private.bi', 'emit.bas', 'emit_x86.bas', 'emit_SSE.bas')}
    return check_text(files)


def require(pattern, text, purpose):
    match = re.search(pattern, text)
    if match is None:
        raise RuntimeError('missing ' + purpose)
    return match


def check_text(files):
    header = files['emit.bi']
    enum = require(r'(?ms)^enum EMIT_NODEOP\n(.*?)^end enum', header, 'opcode enum')[1]
    enum = '\n'.join(line.split("'")[0] for line in enum.splitlines())
    opcodes = re.findall(r'\bEMIT_OP_\w+\b', enum)
    if len(opcodes) != len(set(opcodes)):
        raise RuntimeError('duplicate opcode')
    union = re.search(r'(?ms)^union EMIT_OPCB\n(.*?)^end union', header)
    if not union:
        raise RuntimeError('opcode callback storage must be a typed union')
    fields = dict(re.findall(r'\b(\w+) as EMIT_(\w+)CB', union[1]))
    expected_fields = {'nop', 'bop', 'uop', 'rel', 'stk', 'brc', 'sop', 'lit', 'jtb', 'mem', 'dbg'}
    if set(fields) != expected_fields or any(field.upper() != kind for field, kind in fields.items()):
        raise RuntimeError('callback union members differ')
    if '#assert( sizeof( EMIT_OPCB ) = sizeof( EMIT_NOPCB ) )' not in header:
        raise RuntimeError('one-pointer slot layout assertion is missing')
    kinds = {}
    for name in ('emit_x86.bas', 'emit_SSE.bas'):
        text = files[name]
        for match in re.finditer(r'(?im)^(?:private\s+)?sub\s+(_emit\w+)\b', text):
            end = text.index('\nend sub', match.end())
            assertion = re.search(r'ASSERT_PROC_DECL\(\s*EMIT_(\w+)CB\s*\)', text[match.end():end])
            if assertion:
                kinds[match[1].lower()] = assertion[1].lower()
    initial = require(r'(?ms)^private sub hInitOpFnTB\( \)\n(.*?)^end sub', files['emit_x86.bas'], 'base table initializer')[1]
    assignments = re.findall(r'_opFnTB\((EMIT_OP_\w+)\)\.(\w+) = (@_emit\w+|NULL)', initial)
    if len(assignments) != len(opcodes) or {a[0] for a in assignments} != set(opcodes):
        raise RuntimeError('every opcode must have one explicit slot assignment')
    disabled = {'EMIT_OP_DIVL', 'EMIT_OP_MODF', 'EMIT_OP_MODL', 'EMIT_OP_HADDF',
                'EMIT_OP_SWZREP', 'EMIT_OP_RSQRT', 'EMIT_OP_RCP'}
    slot_kinds = {}
    for opcode, kind, callback in assignments:
        if kind not in expected_fields:
            raise RuntimeError('unknown callback kind: ' + opcode)
        slot_kinds[opcode] = kind
        if callback == 'NULL':
            if opcode not in disabled:
                raise RuntimeError('unexpected disabled callback: ' + opcode)
        elif callback.lower() != ('@_emit' + opcode[len('EMIT_OP_'):]).lower() or kinds.get(callback[1:].lower()) != kind:
            raise RuntimeError('opcode callback or signature differs: ' + opcode)
    if {op for op, _, callback in assignments if callback == 'NULL'} != disabled:
        raise RuntimeError('disabled callback set differs')
    overrides = re.findall(r'_opFnTB_SSE\[(EMIT_OP_\w+)\]\.(\w+) = (@_emit\w+)', files['emit_SSE.bas'])
    if not overrides or len(overrides) != len({o[0] for o in overrides}):
        raise RuntimeError('missing or duplicate SSE override')
    for opcode, kind, callback in overrides:
        expected = '@_emit' + opcode[len('EMIT_OP_'):] + '_SSE'
        if opcode == 'EMIT_OP_SWZREP':
            expected = '@_emitSWZREPF_SSE'
        if callback != expected or slot_kinds.get(opcode) != kind or kinds.get(callback[1:].lower()) != kind:
            raise RuntimeError('SSE callback contract differs: ' + opcode)
    flush = require(r'(?ms)^sub emitFlush\( \)\n(.*?)^end sub', files['emit.bas'], 'opcode dispatch')[1]
    for kind in expected_fields - {'nop'}:
        branch = re.search(r'(?ms)case EMIT_NODECLASS_' + kind.upper() + r'\n(.*?)(?=\n\s*case |\n\s*end select)', flush)
        if not branch or not re.search(r'emit\.opFnTb\[[^\]]+\]\.' + kind + r'\s*\(', branch[1]):
            raise RuntimeError('dispatch member differs: ' + kind)
    constructor = require(r'(?ms)^function emitGasX86_ctor\b(.*?)^end function', files['emit_x86.bas'], 'backend constructor')[1]
    for call in ('hInitOpFnTB( )', 'emit.opFnTb =', '_init_opFnTB_SSE('):
        if call not in constructor:
            raise RuntimeError('backend constructor is missing ' + call)
    if not (constructor.index('hInitOpFnTB( )') < constructor.index('emit.opFnTb =') < constructor.index('_init_opFnTB_SSE(')):
        raise RuntimeError('base table must be reset before SSE selection')
    for operation in ('B2F', 'F2B'):
        delegate = 'emitX86Load' + operation
        if 'declare sub ' + delegate not in files['emit-private.bi'] or 'sub ' + delegate not in files['emit_x86.bas']:
            raise RuntimeError('x87 conversion delegate missing')
        if delegate + '( dvreg, svreg )' not in files['emit_SSE.bas']:
            raise RuntimeError('SSE fallback must call its typed delegate')
    if any('EMIT_CBENTRY' in text or re.search(r'cast\(\s*EMIT_\w+CB\s*,', text, re.I)
           for name, text in files.items() if name.endswith('.bas')):
        raise RuntimeError('erased callback conversion remains')
    return len(opcodes), len(overrides)


def self_test(source):
    files = {name: (source / name).read_text(encoding='utf-8')
             for name in ('emit.bi', 'emit-private.bi', 'emit.bas', 'emit_x86.bas', 'emit_SSE.bas')}
    mutations = [
        ('emit_x86.bas', '_opFnTB(EMIT_OP_LOADI2I).bop = @_emitLOADI2I',
         '_opFnTB(EMIT_OP_LOADI2I).uop = @_emitLOADI2I'),
        ('emit_x86.bas', '\t_opFnTB(EMIT_OP_LOADI2I).bop = @_emitLOADI2I\n', ''),
        ('emit_x86.bas', '_opFnTB(EMIT_OP_ADDI).bop = @_emitADDI',
         '_opFnTB(EMIT_OP_ADDI).bop = @_emitSUBI'),
        ('emit_SSE.bas', '_opFnTB_SSE[EMIT_OP_LOADF2I].bop = @_emitLOADF2I_SSE',
         '_opFnTB_SSE[EMIT_OP_LOADF2I].uop = @_emitLOADF2I_SSE'),
        ('emit_x86.bas', '\thInitOpFnTB( )\n', ''),
        ('emit.bi', 'union EMIT_OPCB', 'type EMIT_OPCB as sub( )\nunion ERASED_CALLBACK')]
    for name, before, after in mutations:
        if files[name].count(before) != 1:
            raise RuntimeError('guard control is no longer unique')
        changed = dict(files)
        changed[name] = changed[name].replace(before, after)
        try:
            check_text(changed)
        except RuntimeError:
            continue
        raise RuntimeError('guard accepted a broken callback contract')
    return len(mutations)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'src/compiler/backend/x86')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    slots, overrides = check(args.source_root)
    print('X86 callback structure: ' + str(slots) + ' typed slots; ' + str(overrides) + ' SSE overrides checked')
    if args.self_test:
        print('X86 callback guard: ' + str(self_test(args.source_root)) + ' broken-contract controls rejected')


if __name__ == '__main__':
    main()

# end of check-x86-emitter-callbacks.py
