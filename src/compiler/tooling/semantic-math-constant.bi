'' Project: FreeBASIC compiler - visible mathematical constant inputs
'' File: tooling/semantic-math-constant.bi
'' Purpose: Observe replacement definitions at the actual numeric input point.
'' Responsibilities: Declare a bounded, read-only macro value query.
'' This file intentionally does NOT contain: rewriting or macro execution.

#ifndef __FB_SEMANTIC_MATH_CONSTANT_BI__
#define __FB_SEMANTIC_MATH_CONSTANT_BI__

declare sub fbSemanticModelMathConstantInputs( byval expression as longint )
declare sub fbSemanticModelMathConstantCounts( )

#endif

'' end of tooling/semantic-math-constant.bi
