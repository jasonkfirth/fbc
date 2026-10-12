/*
    Project: FreeBASIC Aspell qualification
    File: aspell-abi.c
    Purpose: Describe the installed SDK's public layouts and runtime version.
    Responsibilities: Independent C sizes and offsets for the binding.
    This file contains no spelling engine or dictionary modification.
*/
#include <aspell.h>
#include <stddef.h>
#include <stdio.h>
int main(void)
{
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
        sizeof(void *), sizeof(AspellToken), offsetof(AspellToken,len),
        sizeof(AspellKeyInfo), offsetof(AspellKeyInfo,def), offsetof(AspellKeyInfo,desc),
        offsetof(AspellKeyInfo,flags), offsetof(AspellKeyInfo,other_data),
        sizeof(AspellError), sizeof(AspellErrorInfo), offsetof(AspellErrorInfo,num_parms),
        offsetof(AspellErrorInfo,parms));
    printf("VERSION %s\n",aspell_version_string());
    return 0;
}
/* end of aspell-abi.c */
