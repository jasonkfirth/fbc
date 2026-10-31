/*
    Project: FreeBASIC C backend regression tests
    File: c-literal-storage-caller.c
    Purpose: Exercise writable literal addresses through the native C ABI.
    Responsibilities: Mutate caller-owned bytes and overwrite a prior stack frame.
    This file intentionally does not allocate storage or suppress diagnostics.
*/

void literal_storage_write(char *value);
unsigned int literal_storage_clobber(void);

void literal_storage_write(char *value)
{
    value[0] = (char)(value[0] - ('a' - 'A'));
}

unsigned int literal_storage_clobber(void)
{
    /* One page-sized frame overwrites the small prior literal-return frame. */
    /* Volatile accesses retain the overwrite used by the lifetime test. */
    volatile unsigned char bytes[4096];
    unsigned int checksum = 0;

    for (unsigned int i = 0; i < sizeof(bytes); ++i)
        bytes[i] = (unsigned char)i;

    for (unsigned int i = 0; i < sizeof(bytes); ++i)
        checksum += bytes[i];

    return checksum;
}

/* end of c-literal-storage-caller.c */
