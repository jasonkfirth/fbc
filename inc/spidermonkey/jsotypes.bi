''
''
'' jsotypes -- header translated with help of SWIG FB wrapper
'' File: jsotypes.bi
'' Purpose: Provide the old API's scalar aliases using the C type definitions.
'' Responsibilities: Keep int32 and uint32 consistent with jstypes.bi.
'' This file contains no tagged-value operations or engine implementation.
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __jsotypes_bi__
#define __jsotypes_bi__

type uint as JSUintn
type uintn as JSUintn
type uint64 as JSUint64
type uint32 as JSUint32
type uint16 as JSUint16
type uint8 as JSUint8
type intn as JSIntn
type int64 as JSInt64
type int32 as JSInt32
type int16 as JSInt16
type int8 as JSInt8
type float64 as JSFloat64

#endif
'' end of jsotypes.bi
