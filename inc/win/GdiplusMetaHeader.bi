''
''
'' GdiplusMetaHeader -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __win_GdiplusMetaHeader_bi__
#define __win_GdiplusMetaHeader_bi__

type ENHMETAHEADER3
	iType as DWORD
	nSize as DWORD
	rclBounds as RECTL
	rclFrame as RECTL
	dSignature as DWORD
	nVersion as DWORD
	nBytes as DWORD
	nRecords as DWORD
	nHandles as WORD
	sReserved as WORD
	nDescription as DWORD
	offDescription as DWORD
	nPalEntries as DWORD
	szlDevice as SIZEL
	szlMillimeters as SIZEL
end type

type PWMFRect16 field=2
	Left as INT16
	Top as INT16
	Right as INT16
	Bottom as INT16
end type

type WmfPlaceableFileHeader field=2
	Key as UINT32
	Hmf as INT16
	BoundingBox as PWMFRect16
	Inch as INT16
	Reserved as UINT32
	Checksum as INT16
end type

#define GDIP_EMFPLUSFLAGS_DISPLAY &h00000001

type MetafileHeader
	Type_ as MetafileType
	Size as UINT
	Version as UINT
	EmfPlusFlags as UINT
	DpiX as REAL
	DpiY as REAL
	X as INT_
	Y as INT_
	Width as INT_
	Height as INT_
	EmfPlusHeaderSize as INT_
	LogicalDpiX as INT_
	LogicalDpiY as INT_

	declare function GetType () as MetafileType
	declare function GetMetafileSize () as UINT
	declare function GetVersion () as UINT
	declare function GetEmfPlusFlags () as UINT
	declare function GetDpiX () as REAL
	declare function GetDpiY () as REAL
	declare sub GetBounds (byval rect as Rect ptr)
	declare function IsWmf () as BOOL
	declare function IsWmfPlaceable () as BOOL
	declare function IsEmf () as BOOL
	declare function IsEmfOrEmfPlus () as BOOL
	declare function IsEmfPlus () as BOOL
	declare function IsEmfPlusDual () as BOOL
	declare function IsEmfPlusOnly () as BOOL
	declare function IsDisplay () as BOOL
	declare function GetWmfHeader () as METAHEADER ptr
	declare function GetEmfHeader () as ENHMETAHEADER3 ptr
end type

private function MetafileHeader.GetType () as MetafileType
	return this.Type_
end function

private function MetafileHeader.GetMetafileSize () as UINT
	return this.Size
end function

private function MetafileHeader.GetVersion () as UINT
	return this.Version
end function

private function MetafileHeader.GetEmfPlusFlags () as UINT
	return this.EmfPlusFlags
end function

private function MetafileHeader.GetDpiX () as REAL
	return this.DpiX
end function

private function MetafileHeader.GetDpiY () as REAL
	return this.DpiY
end function

private sub MetafileHeader.GetBounds (byval rect as Rect ptr)
	if( rect <> NULL ) then
		rect->X = this.X
		rect->Y = this.Y
		rect->Width = this.Width
		rect->Height = this.Height
	end if
end sub

private function MetafileHeader.IsWmf () as BOOL
	return (this.Type_ = MetafileTypeWmf) or (this.Type_ = MetafileTypeWmfPlaceable)
end function

private function MetafileHeader.IsWmfPlaceable () as BOOL
	return this.Type_ = MetafileTypeWmfPlaceable
end function

private function MetafileHeader.IsEmf () as BOOL
	return this.Type_ = MetafileTypeEmf
end function

private function MetafileHeader.IsEmfOrEmfPlus () as BOOL
	return (this.Type_ = MetafileTypeEmf) or this.IsEmfPlus()
end function

private function MetafileHeader.IsEmfPlus () as BOOL
	return (this.Type_ = MetafileTypeEmfPlusOnly) or (this.Type_ = MetafileTypeEmfPlusDual)
end function

private function MetafileHeader.IsEmfPlusDual () as BOOL
	return this.Type_ = MetafileTypeEmfPlusDual
end function

private function MetafileHeader.IsEmfPlusOnly () as BOOL
	return this.Type_ = MetafileTypeEmfPlusOnly
end function

private function MetafileHeader.IsDisplay () as BOOL
	return (this.EmfPlusFlags and GDIP_EMFPLUSFLAGS_DISPLAY) <> 0
end function

private function MetafileHeader.GetWmfHeader () as METAHEADER ptr
	return NULL
end function

private function MetafileHeader.GetEmfHeader () as ENHMETAHEADER3 ptr
	return NULL
end function

#endif
