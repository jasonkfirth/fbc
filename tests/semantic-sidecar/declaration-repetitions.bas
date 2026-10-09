'' Project: FreeBASIC semantic declaration tests
'' File: declaration-repetitions.bas
'' Purpose: Preserve accepted repetition decisions before canonical state changes.
'' Responsibilities: Exact extern shapes, aliases, overloads and macro origins.
'' This file intentionally does NOT link unresolved external objects.
#lang "fb"

extern ScalarValue as long
extern ScalarValue as long
dim shared ScalarValue as long
type NumberAlias as long
type OtherLong as long
type NumberAlias as OtherLong
type FixedAlias as string * 4
type FixedAlias as string * 8
type FixedAlias as string * 4

extern RankValue() as long
extern RankValue(any) as long
extern RankValue(any) as long
extern FixedValue(0 to 3) as long
extern FixedValue(0 to 3) as long

#define REPEAT_ALIAS type NumberAlias as long
REPEAT_ALIAS
#define REPEAT_EXTERN extern ScalarMacro as long
REPEAT_EXTERN
REPEAT_EXTERN

declare sub FirstProcedure(byval value as long)
declare sub OverloadedProcedure overload(byval value as long)
declare sub OverloadedProcedure overload(byval value as string)

namespace AnotherScope
    type NumberAlias as long
    extern ScalarValue as long
end namespace

'' end of declaration-repetitions.bas
