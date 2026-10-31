'' Project: FreeBASIC compiler tests
'' File: va_cva_aggregate.bas
'' Purpose: Exercise C aggregate varargs, fixed CLongDouble calls, and literals.
'' Responsibilities: Cover register classes, stack overflow, fixed C arguments,
'' results, exact long-double literals, and native formatting.
'' This file contains no platform ABI emulation or native C test driver.
#lang "fb"

#include once "crt/longdouble.bi"
#include once "crt/stdio.bi"
#include once "crt/stdarg.bi"

#ifdef __FB_WIN32__
'' The Microsoft CRT reads %L floating arguments as Double. MinGW's ANSI
'' formatter accepts the native CLongDouble value exercised by this test.
Declare Function FormatLongDouble CDecl Alias "__mingw_snprintf" _
    (ByVal buffer As ZString Ptr, ByVal capacity As size_t, _
     ByVal format As ZString Ptr, ...) As Long
#else
#define FormatLongDouble snprintf
#endif

Type AggregateRR
    first_value As LongInt
    second_value As LongInt
End Type

Type AggregateSmall
    first_value As Long
    second_value As Long
End Type

Type AggregateX
    floating_value As Double
End Type

Type AggregateRX
    integer_part As LongInt
    floating_part As Double
End Type

Type AggregateXR
    floating_part As Double
    integer_part As LongInt
End Type

Type AggregateXX
    first_value As Double
    second_value As Double
End Type

Type AggregateMemory
    first_value As LongInt
    second_value As LongInt
    third_value As LongInt
End Type

Private Function DecodeCvaAggregates (ByVal arguments As Cva_List) As LongInt
    Dim As AggregateRR rr = Cva_Arg(arguments, AggregateRR)
    Dim As AggregateSmall small_value = Cva_Arg(arguments, AggregateSmall)
    Dim As AggregateX x_value = Cva_Arg(arguments, AggregateX)
    Dim As AggregateRX rx = Cva_Arg(arguments, AggregateRX)
    Dim As AggregateXR xr = Cva_Arg(arguments, AggregateXR)
    Dim As AggregateXX xx = Cva_Arg(arguments, AggregateXX)
    Dim As AggregateMemory memory_value = Cva_Arg(arguments, AggregateMemory)

    Return rr.first_value * 1000000000000 + rr.second_value * 100000000000 + _
           small_value.first_value * 10000000000 + small_value.second_value * 1000000000 + _
           Cast(LongInt, x_value.floating_value) * 100000000 + _
           rx.integer_part * 10000000 + Cast(LongInt, rx.floating_part) * 1000000 + _
           Cast(LongInt, xr.floating_part) * 100000 + xr.integer_part * 10000 + _
           Cast(LongInt, xx.first_value) * 1000 + Cast(LongInt, xx.second_value) * 100 + _
           memory_value.first_value * 10 + memory_value.second_value + _
           memory_value.third_value
End Function

Function ReadCvaAggregates CDecl Alias "read_cva_aggregates" _
    (ByVal arguments As Cva_List) As LongInt
    Return DecodeCvaAggregates(arguments)
End Function

Function ReadCvaLongDouble CDecl Alias "read_cva_longdouble" _
    (ByVal arguments As Cva_List) As Integer
    Dim As CLongDouble value = fb_LongDoubleVaArg(arguments)
    Return -CInt(value = 1.75#)
End Function

Function ReadOwnCvaAggregates CDecl _
    (ByVal marker As LongInt, ...) As LongInt
    Dim As Cva_List arguments
    Cva_Start(arguments, marker)
    Dim As LongInt result = DecodeCvaAggregates(arguments)
    Cva_End(arguments)
    Return result
End Function

Function ReadOwnCvaAggregatesOverflow CDecl _
    (ByVal first As LongInt, ByVal second As LongInt, _
     ByVal third As LongInt, ByVal fourth As LongInt, _
     ByVal fifth As LongInt, ByVal marker As LongInt, ...) As LongInt
    Dim As Cva_List arguments
    Cva_Start(arguments, marker)
    Dim As LongInt result = DecodeCvaAggregates(arguments)
    Cva_End(arguments)
    Return result
End Function

Declare Function cva_aggregate_registers CDecl _
    Alias "cva_aggregate_registers" () As LongInt
Declare Function cva_aggregate_overflow CDecl _
    Alias "cva_aggregate_overflow" () As LongInt
Declare Function cva_longdouble_register CDecl _
    Alias "cva_longdouble_register" () As Integer
Declare Sub cva_write_longdouble_precision CDecl _
    Alias "cva_write_longdouble_precision" (ByVal destination As Any Ptr)
#if defined( __FB_X86__ ) and defined( __FB_LINUX__ )
Declare Function cva_longdouble_identity CDecl _
    Alias "cva_longdouble_identity" (ByVal value As CLongDouble) As CLongDouble
Declare Function cva_longdouble_transform CDecl _
    Alias "cva_longdouble_transform" (ByVal value As CLongDouble) As CLongDouble
Declare Function cva_longdouble_add CDecl _
    Alias "cva_longdouble_add" (ByVal lhs As CLongDouble, _
                                 ByVal rhs As CLongDouble) As CLongDouble
Declare Function cva_call_fbc_longdouble_export CDecl _
    Alias "cva_call_fbc_longdouble_export" (ByVal value As CLongDouble) As CLongDouble
#endif

Function fbc_cva_longdouble_export CDecl Alias "fbc_cva_longdouble_export" _
    (ByVal value As CLongDouble) As CLongDouble
    Return value + 0.5#
End Function

Dim As AggregateRR rr = Type<AggregateRR>(12, 34)
Dim As AggregateSmall small_value = Type<AggregateSmall>(11, 22)
Dim As AggregateX x_value = Type<AggregateX>(13.0)
Dim As AggregateRX rx = Type<AggregateRX>(11, 25.0)
Dim As AggregateXR xr = Type<AggregateXR>(12.0, 34)
Dim As AggregateXX xx = Type<AggregateXX>(56.0, 78.0)
Dim As AggregateMemory memory_value = Type<AggregateMemory>(9, 8, 7)

If cva_aggregate_registers() <> 15533436603905 Then End 1
If cva_aggregate_overflow() <> 1235534567911 Then End 2
If ReadOwnCvaAggregates(0, rr, small_value, x_value, rx, xr, xx, memory_value) <> _
    15533436603905 Then End 3

rr = Type<AggregateRR>(1, 2)
small_value = Type<AggregateSmall>(3, 4)
x_value = Type<AggregateX>(15.0)
rx = Type<AggregateRX>(3, 4.0)
xr = Type<AggregateXR>(5.0, 6)
xx = Type<AggregateXX>(7.0, 8.0)
memory_value = Type<AggregateMemory>(9, 10, 11)
If ReadOwnCvaAggregatesOverflow(1, 2, 3, 4, 5, 6, rr, small_value, x_value, _
                                rx, xr, xx, memory_value) <> 1235534567911 Then End 4
If cva_longdouble_register() <> 1 Then End 5

Dim As CLongDouble literal_value = CLongDoubleFromStringValue( _
    StrPtr(!"1.0000000000000001"))
Dim As CLongDouble c_literal_value
cva_write_longdouble_precision(@c_literal_value)
If (literal_value = c_literal_value) <> -1 Then End 10
If (literal_value > CLongDoubleFromStringValue(StrPtr(!"1.0"))) <> -1 Then End 11
Dim As CLongDouble fractional_value = _
    CLongDoubleFromStringValue(StrPtr(!"1.75"))
If Cast(Long, fractional_value) <> 1 Then End 12
If Cast(Long, CLongDoubleFromStringValue(StrPtr(!"-1.75"))) <> -1 Then End 13
If Cast(ULong, fractional_value) <> 1 Then End 14

Dim As CLongDouble fixed_input = CLongDoubleFromDoubleValue(1.75#)
#if defined( __FB_X86__ ) and defined( __FB_LINUX__ )
If Cast(Double, cva_longdouble_identity(fixed_input)) <> 1.75# Then End 6
If Cast(Double, cva_longdouble_transform(fixed_input)) <> 4.0# Then End 7
If Cast(Double, cva_longdouble_add( _
    fixed_input, CLongDoubleFromDoubleValue(0.5#))) <> 2.25# Then End 15
If Cast(Double, cva_call_fbc_longdouble_export(fixed_input)) <> 2.25# Then End 8
#endif

Dim As ZString * 32 formatted_value
Dim As CLongDouble formatting_value = CLongDoubleFromDoubleValue(1.75#)
Dim As Integer format_length = FormatLongDouble( _
    @formatted_value, SizeOf(formatted_value), "%Lg", formatting_value)
If format_length <> 4 OrElse formatted_value <> "1.75" Then End 9

Print "CVA_AGGREGATE=PASS classes=R,X,RR,RX,XR,XX,memory,longdouble sources=C,FreeBASIC paths=register,overflow,format"
End 0

'' end of va_cva_aggregate.bas
