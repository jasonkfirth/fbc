'' Project: FreeBASIC C backend regression tests
'' File: c-vtable-abi.bi
'' Purpose: Share virtual method declarations across native emission units.
'' Responsibilities: Define inherited, abstract and differently typed slots.
'' This file intentionally does NOT implement methods or inspect C spelling.
#ifndef C_VTABLE_ABI_BI
#define C_VTABLE_ABI_BI

extern vtable_destroyed as long

type TableBase extends object
	marker as long
	declare constructor()
	declare virtual function number(byval value as long) as long
	declare virtual function fraction(byval value as double) as double
	declare virtual function reference() byref as long
	declare virtual destructor()
end type

type TableAbstract extends TableBase
	declare abstract function number(byval value as long) as long
	declare virtual destructor()
end type

type TableLeaf extends TableAbstract
	declare constructor()
	declare function number(byval value as long) as long override
	declare function fraction(byval value as double) as double override
	declare virtual destructor()
end type

#endif
'' end of c-vtable-abi.bi
