<!--
Project: FreeBASIC semantic tooling
File: semantic-boolean-loop-inputs.md
Purpose: Describe compiler-owned inputs used by Boolean and loop policies.
Responsibilities: Constant destinations, selected enum operands and source coordinates.
This file intentionally does NOT define lint policy, infer purity or add wide constant evaluation.
-->

# Boolean and loop semantic inputs

The compiler exports observations; consumers decide which source patterns
deserve diagnostics. The following additions preserve original identities
without changing ordinary program code or the datatype ABI.

## Constant initializer destinations

`constant-initializer-targets` covers successfully created CONST symbols in a
full export. The CONST parser saves the original expression identity before
conversion can replace or free its AST node. After successful symbol creation,
it publishes that identity with the selected destination type and initializer
role. A failed declaration does not advertise a successful destination.

Consumers must also require the expression, assignment and context capabilities
they use. This capability alone does not make a reduced or damaged graph a
complete input. A Boolean literal policy can consequently distinguish a written
integer literal from a named constant or computed expression, including after
folding. FreeBASIC Boolean True remains minus one.

This observer does not make lowered signed or unsigned 128-bit conversions
compile-time constants. The existing CONST value-carrier guard still rejects
those unsupported initializers before accessing the AST value union.

## Selected numeric operands with enum sources

`enum-selected-numeric-operands` extends the original side of selected numeric
operand observations. An original enum expression retains its enum declaration
identity. Its selected snapshot must still be a primitive numeric expression
with no subtype, matching the dtype chosen by the actual operator builder.

A reader admits an original enum only when the capability is available and its
subtype names an enum declaration in the same compilation module. Pointer enums,
foreign declarations, incomplete selection groups and contradictory selected
types remain invalid. Enum names or enumerator values do not establish a
different arithmetic representation.

This distinction matters for floating comparisons. A loop predicate can use
floating arithmetic after an enum-to-floating coercion even though its original
source operand is an enum. Looking only at the source spelling or original
datatype loses the selected computation domain.

## Proved coordinate prefixes

Coordinate conversion proves the prefix ending at the requested decoder
boundary. Unknown bytes later on the physical line do not invalidate an earlier
proved token. This changes source-coordinate observations, not program code.

An accepted comment whose marker cannot be mapped retains an `unverified` LOC
with both byte offsets set to minus one. Its module, marker role, same-line range
and marker width still require validation. Such a record supplies no editable
byte span. Consumers needing physical text must require a mapped location.

The lexer discards the tail of a physical line after a lone continuation
underscore. A suppression directive placed there is not an accepted comment.
Use a real preceding comment with `disable-next-line` for a diagnostic anchored
at that continued statement's opening token.

## Qualification and ownership

The composed semantic image rebuilds four observer objects and reuses 185
qualified compiler objects. It uses the existing stable-v1 semantic-product
default; ordinary compiler builds retain native defaults and explicit datatype
ABI overrides remain available. Runtime libraries are unchanged.

The focused qualification compares twelve ordinary C emissions across win32 and
win64, with capture off, text capture and binary capture. Parent and composed
code and diagnostics are identical. Twelve paired compile-fail cases preserve
the ordinary CONST rejection with capture off and on. Native and independent
readers also test gated enum acceptance and malformed alternatives.

These are observer and consumer contracts. They are not a claim of whole-program
termination analysis, interprocedural purity, runtime-library rebuilding or
complete platform qualification.

<!-- end of semantic-boolean-loop-inputs.md -->
