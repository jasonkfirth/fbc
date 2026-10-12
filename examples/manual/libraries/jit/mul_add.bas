'' examples/manual/libraries/jit/mul_add.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'libjit'
'' Purpose: Compile and call a C-style integer multiply/add function.
'' Responsibilities: Typed arguments, build locking and owned context cleanup.
'' This example contains no persistent machine code or native-float ABI calls.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibJit
'' --------

'' Simple mul/add example

#include "jit.bi"

' The caller owns the build lock and releases the context on failure.
Function emit_mul_add(ByVal mul_add As jit_function_t) As Integer
	' build function (result = (x*y)+z)
	Dim As jit_value_t x, y, z, temp1, temp2
	x = jit_value_get_param(mul_add, 0)
	y = jit_value_get_param(mul_add, 1)
	z = jit_value_get_param(mul_add, 2)
	If x = 0 OrElse y = 0 OrElse z = 0 Then Return 0
	temp1 = jit_insn_mul(mul_add, x, y)
	If temp1 = 0 Then Return 0
	temp2 = jit_insn_add(mul_add, temp1, z)
	If temp2 = 0 Then Return 0
	If jit_insn_return(mul_add, temp2) = 0 Then Return 0

	' compile function
	Return jit_function_compile(mul_add)
End Function

' initialize libjit
Dim context As jit_context_t = jit_context_create()
If context = 0 Then End 1
jit_context_build_start(context)

' define function mul_add(x, y, z)
Dim params(0 To 2) As jit_type_t = {jit_type_int, jit_type_int, jit_type_int}
Dim signature As jit_type_t = jit_type_create_signature( _
	jit_abi_cdecl,  _ ' C-style function
	jit_type_int,   _ ' Return type
	@params(0),     _ ' Parameter array
	3,              _ ' Number of components
	1               _ ' Count references?
)
If signature = 0 Then
	jit_context_build_end(context)
	jit_context_destroy(context)
	End 1
End If
Dim mul_add As jit_function_t = jit_function_create(context, signature)
'' The function retains its own signature reference.
jit_type_free(signature)
If mul_add = 0 Then
	jit_context_build_end(context)
	jit_context_destroy(context)
	End 1
End If

Dim As Integer build_ok = emit_mul_add(mul_add)
jit_context_build_end(context)
If build_ok = 0 Then
	jit_context_destroy(context)
	End 1
End If

' call function
'' jit_type_int is a C int, even when FreeBASIC Integer is 64 bits.
Dim As jit_int a = 3
Dim As jit_int b = 5
Dim As jit_int c = 2
Dim As jit_int result
Dim args(0 To 2) As jit_int Ptr = {@a, @b, @c}
If jit_function_apply(mul_add, @args(0), @result) = 0 Then
	jit_context_destroy(context)
	End 1
End If
Print Using "mul__add(&, &, &) = &"; a; b; c; result

' clean up libjit
jit_context_destroy(context)
'' end of mul_add.bas
