'' FreeBASIC bindings for the shared Direct3D 9 effect type declarations
''
'' File: d3dx9effecttypes.bi
''
'' Purpose:
''   Keep effect parameter handles and classifications available to both the
''   effect and shader headers without making either header include the
''   umbrella d3dx9.bi file.
''
'' This file intentionally does not contain effect or shader interfaces.

#pragma once

#ifndef __D3DX9_EFFECT_TYPES_BI__
#define __D3DX9_EFFECT_TYPES_BI__

type D3DXHANDLE as const zstring ptr
type LPD3DXHANDLE as D3DXHANDLE ptr

type _D3DXMACRO
	Name as const zstring ptr
	Definition as const zstring ptr
end type
type D3DXMACRO as _D3DXMACRO
type LPD3DXMACRO as _D3DXMACRO ptr

type D3DXPARAMETER_CLASS as long
enum
	D3DXPC_SCALAR
	D3DXPC_VECTOR
	D3DXPC_MATRIX_ROWS
	D3DXPC_MATRIX_COLUMNS
	D3DXPC_OBJECT
	D3DXPC_STRUCT
	D3DXPC_FORCE_DWORD = &h7fffffff
end enum
type LPD3DXPARAMETER_CLASS as D3DXPARAMETER_CLASS ptr

type D3DXPARAMETER_TYPE as long
enum
	D3DXPT_VOID
	D3DXPT_BOOL
	D3DXPT_INT
	D3DXPT_FLOAT
	D3DXPT_STRING
	D3DXPT_TEXTURE
	D3DXPT_TEXTURE1D
	D3DXPT_TEXTURE2D
	D3DXPT_TEXTURE3D
	D3DXPT_TEXTURECUBE
	D3DXPT_SAMPLER
	D3DXPT_SAMPLER1D
	D3DXPT_SAMPLER2D
	D3DXPT_SAMPLER3D
	D3DXPT_SAMPLERCUBE
	D3DXPT_PIXELSHADER
	D3DXPT_VERTEXSHADER
	D3DXPT_PIXELFRAGMENT
	D3DXPT_VERTEXFRAGMENT
	D3DXPT_UNSUPPORTED
	D3DXPT_FORCE_DWORD = &h7fffffff
end enum
type LPD3DXPARAMETER_TYPE as D3DXPARAMETER_TYPE ptr

#endif

'' end of d3dx9effecttypes.bi
