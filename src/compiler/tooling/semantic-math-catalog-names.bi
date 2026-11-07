'' Project: FreeBASIC compiler - mathematical catalogue inputs
'' File: tooling/semantic-math-catalog-names.bi
'' Purpose: Supply the reviewed query names without guessing source identifiers.
'' Responsibilities: Stable catalogue ordinal to actual preprocessor lookup name.
'' This file intentionally does NOT assume a named macro exists.
#ifndef __FB_SEMANTIC_MATH_CATALOG_NAMES_BI__
#define __FB_SEMANTIC_MATH_CATALOG_NAMES_BI__
private function hMathCatalogName(byval ordinal as integer) as string
	select case ordinal
	case 0: return "M_E"
	case 1: return "M_1_PI"
	case 2: return "M_SQRT1_2"
	case 3: return "M_2_PI"
	case 4: return "M_2_SQRTPI"
	case 5: return "M_PI_2"
	case 6: return "M_PI_3"
	case 7: return "M_PI_4"
	case 8: return "M_PI_6"
	case 9: return "M_PI_8"
	case 10: return "M_LN2"
	case 11: return "M_LN10"
	case 12: return "M_LOG2_10"
	case 13: return "M_LOG2E"
	case 14: return "M_LOG10_2"
	case 15: return "M_LOG10E"
	case 16: return "M_PI"
	case 17: return "M_SQRT2"
	case 18: return "M_TAU"
	case 19: return "M_EULER_GAMMA"
	case 20: return "M_PHI"
	end select
	return ""
end function
#endif
'' end of tooling/semantic-math-catalog-names.bi
