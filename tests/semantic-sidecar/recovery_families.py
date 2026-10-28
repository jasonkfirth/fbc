"""Project: FreeBASIC semantic recovery tests
File: recovery_families.py
Purpose: Exercise native recovery roots across parser subsystems.
Responsibilities: Required native routes, ordinary defaults and diagnostic parity.
This file intentionally does NOT treat a source hook match as runtime evidence.
"""

from recovery_steps import recovered

CASES = (
    ('const-missing', 'const missing as long =\nprint 1\n', 'parser-decl-const.bas'),
    ('const-nonconstant', 'dim value as long\nconst wrong = value\n', 'parser-decl-const.bas'),
    ('const-string', 'const wrong as string = 42\n', 'parser-decl-const.bas'),
    ('param-incomplete', 'declare sub Sample(byref value as any = 1)\n', 'parser-decl-proc-params.bas'),
    ('struct-field', 'type T field =\nmember as long\nend type\n', 'parser-decl-struct.bas'),
    ('initializer', 'dim value as long = "wrong"\n', 'parser-decl-symb-init.bas'),
    ('const-type-expression', 'dim value as integer<MissingBits>\n', 'parser-decl-symbtype.bas'),
    ('const-type-variable', 'dim bits as long\ndim value as integer<bits>\n', 'parser-decl-symbtype.bas'),
    ('byref-literal', 'dim referenced as string\ndim byref value as long = referenced\n', 'parser-decl-var.bas'),
    ('auto-missing', 'var missing =\n', 'parser-decl-var.bas'),
    ('ellipsis-lower', 'dim values(... to 3) as long\n', 'parser-decl-var.bas'),
    ('atom-parentheses', 'print ()\n', 'parser-expr-atom.bas'),
    ('atom-base', 'print base.member\n', 'parser-expr-atom.bas'),
    ('array-index', 'dim values(0 to 2) as long\nprint values("wrong")\n', 'parser-expr-variable.bas'),
    ('array-intrinsic', 'print ubound(123)\n', 'parser-quirk-array.bas'),
    ('cast-intrinsic', 'print cint()\n', 'parser-quirk-casting.bas'),
    ('console-range', 'view print 1\n', 'parser-quirk-console.bas'),
    ('file-close', 'close 1,\n', 'parser-quirk-file.bas'),
    ('file-put', 'put #1, , 123\n', 'parser-quirk-file.bas'),
    ('file-open', 'open "never-opened" for binary as #1 len\n', 'parser-quirk-file.bas'),
    ('graphics-pointer', 'type ImageOwner\npixels(0 to 3) as ulong\nend type\ndeclare function ImageValue() as ImageOwner ptr\nget (0,0)-(1,1), ImageValue()->pixels(0)\n', 'parser-quirk-gfx.bas'),
    ('iif-types', 'dim condition as long\nprint iif(condition, "wrong", 2)\n', 'parser-quirk-iif.bas'),
    ('math-types', 'print sin("wrong")\n', 'parser-quirk-math.bas'),
    ('memory-elements', 'dim values as long ptr = new long["wrong"]\n', 'parser-quirk-mem.bas'),
    ('on-error-gosub', '#lang "fblite"\non error gosub Handler\nHandler:\n', 'parser-quirk-on.bas'),
    ('peek-address', 'print peek(long, )\n', 'parser-quirk-peekpoke.bas'),
    ('string-format', 'type T\nvalue as long\nend type\ndim item as T\nprint str(item)\n', 'parser-quirk-string.bas'),
    ('string-instr', 'print instr(1,2,3)\n', 'parser-quirk-string.bas'),
    ('vararg-c', 'sub Variadic cdecl(byval first as long, ...)\nprint va_first()\nend sub\n', 'parser-quirk-vafirst.bas'),
    ('call-arguments', 'declare sub Called(byval value as long)\nCalled(,)\n', 'parser-proccall-args.bas'),
    ('byref-result', 'function ResultReference() byref as long\nreturn "wrong"\nend function\n', 'parser-proccall.bas'),
    ('do-condition', 'do while\nloop\n', 'parser-compound-do.bas'),
    ('for-initial', 'for index as long = to 3\nnext\n', 'parser-compound-for.bas'),
    ('for-final', 'for index as long = 1 to\nnext\n', 'parser-compound-for.bas'),
    ('for-step', 'for index as long = 1 to 3 step\nnext\n', 'parser-compound-for.bas'),
    ('if-condition', 'if then\nend if\n', 'parser-compound-if.bas'),
    ('select-condition', 'select case\nend select\n', 'parser-compound-select.bas'),
    ('select-bound', 'select case 1\ncase\nend select\n', 'parser-compound-select.bas'),
    ('select-constant-bound', 'select case as const\nend select\n', 'parser-compound-select-const.bas'),
    ('while-condition', 'while\nwend\n', 'parser-compound-while.bas'),
)


def check_families(test):
    for name, text, native_file in CASES:
        with test.subTest(recovery_family=name):
            source = test.source(text, "recovery-family-" + name + ".bas")
            baseline, _ = test.invoke([source], mode="off", success=False)
            result, model = recovered(test, source)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            steps = model.recovery_steps.get(1, ())
            test.assertTrue(any(step[2].endswith(native_file) for step in steps), (name, steps))


def check_defaults(test):
    text = ("for index as long = 1 to 3\nnext\n"
            'open "never-executed" for binary as #1\n'
            'print iif(1, "chosen", 2)\n')
    source = test.source(text, "ordinary-default-roots.bas")
    for mode in ("full", "expressions", "bindings"):
        with test.subTest(mode=mode):
            model = test.compile(source, mode=mode)
            test.assertFalse(model.recovery_steps)

# end of recovery_families.py
