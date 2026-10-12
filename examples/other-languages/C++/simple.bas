'' Project: FreeBASIC C++ interoperability examples
'' File: simple.bas
'' Purpose: Call the sum function in the library built from simple.cxx.
'' Responsibilities: Match the C++ namespace and C int parameter types.
'' This file contains no implementation of the C++ library.

extern "c++" lib "simple"
	namespace cpp
		'' C++ int remains 32 bits on both 32-bit and 64-bit targets.
		declare function sum( byval a as long, byval b as long ) as long
	end namespace
end extern

print "1 + 2 ="; cpp.sum( 1, 2 )

'' end of simple.bas
