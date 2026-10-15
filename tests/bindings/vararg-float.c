/*
    FreeBASIC native variadic floating-point regression
    File: vararg-float.c
    Purpose: Read floating arguments through the genuine platform C ABI.
    Responsibilities: Register and stack argument checks using stdarg.h.
    This file contains no compiler emitter or substitute calling convention.
*/
#include <stdarg.h>
#include <stdio.h>

static int check(va_list *arguments)
{
    static const double expected[] = {1.25, 0.0, -3.75, 4.125, 5.5};
    unsigned i;
    for(i=0; i<sizeof(expected)/sizeof(expected[0]); ++i) {
        double value=va_arg(*arguments,double);
        if(value!=expected[i]) {
            fprintf(stderr,"VARARG_FLOAT mismatch index=%u expected=%g actual=%g\n",i,expected[i],value);
            return 1;
        }
    }
    return 0;
}

int vararg_float1(int first,...)
{
    va_list arguments;int result;
    if(first!=11) return 2;
    va_start(arguments,first);result=check(&arguments);va_end(arguments);return result;
}
int vararg_float2(int first,int second,...)
{
    va_list arguments;int result;
    if(first!=11 || second!=22) return 2;
    va_start(arguments,second);result=check(&arguments);va_end(arguments);return result;
}
int vararg_float3(int first,int second,int third,...)
{
    va_list arguments;int result;
    if(first!=11 || second!=22 || third!=33) return 2;
    va_start(arguments,third);result=check(&arguments);va_end(arguments);return result;
}
int vararg_float_single(float first,int marker,...)
{
    va_list arguments;int result;
    if(first!=1.25f || marker!=22) return 2;
    va_start(arguments,marker);result=check(&arguments);va_end(arguments);return result;
}
int vararg_float_double(double first,int marker,...)
{
    va_list arguments;int result;
    if(first!=1.25 || marker!=22) return 2;
    va_start(arguments,marker);result=check(&arguments);va_end(arguments);return result;
}
/* end of vararg-float.c */
