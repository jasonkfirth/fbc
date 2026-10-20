/*
    FreeBASIC classic AmigaOS metadata qualification
    ------------------------------------------------
    File: file-info-bridge.c
    Purpose: Observe DOS metadata buffer alignment in actual runtime calls.
    Responsibilities: Guard native query vectors, exercise both legal stack
        alignments, restore all vectors, and remove only owned fixtures.
    This file intentionally does NOT contain runtime filesystem replacements.
*/

#include "../../src/rtlib/fb.h"
#include <stddef.h>
#include <sys/stat.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>

APTR amiga_fib_old_examine;
APTR amiga_fib_old_examine_fh;
APTR amiga_fib_old_next;
struct Task *amiga_fib_probe_task;
volatile unsigned long amiga_fib_calls;
volatile unsigned long amiga_fib_rejected;

extern ULONG amiga_fib_examine(void);
extern ULONG amiga_fib_examine_fh(void);
extern ULONG amiga_fib_next(void);
extern int amiga_fib_invoke(int (*operation)(void), unsigned long alignment);

/* The hooks inspect only this probe's task. Other native tasks continue
   through the original entry points without observation or interception.
   The NDK puts ThisTask at byte 276; fail compilation if its layout changes.
   DOS Examine, ExamineFH and ExNext all receive the FIB address in D2. */
_Static_assert(offsetof(struct ExecBase, ThisTask) == 276, "ExecBase layout changed");
_Static_assert(sizeof(FB_AMIGA_FILE_INFO) == sizeof(struct FileInfoBlock), "FIB layout changed");
_Static_assert(__alignof__(FB_AMIGA_FILE_INFO) == 4, "FIB alignment changed");

__asm__(
    ".text\n.align 2\n"
    ".globl _amiga_fib_examine\n"
    "_amiga_fib_examine:\n"
    "move.l _amiga_fib_old_examine,-(sp)\n"
    "bra amiga_fib_check\n"
    ".globl _amiga_fib_examine_fh\n"
    "_amiga_fib_examine_fh:\n"
    "move.l _amiga_fib_old_examine_fh,-(sp)\n"
    "bra amiga_fib_check\n"
    ".globl _amiga_fib_next\n"
    "_amiga_fib_next:\n"
    "move.l _amiga_fib_old_next,-(sp)\n"
    "amiga_fib_check:\n"
    "movem.l d0/a0,-(sp)\n"
    "movea.l _SysBase,a0\n"
    "movea.l 276(a0),a0\n"
    "cmpa.l _amiga_fib_probe_task,a0\n"
    "bne amiga_fib_forward\n"
    "addq.l #1,_amiga_fib_calls\n"
    "move.l d2,d0\n"
    "andi.l #3,d0\n"
    "bne amiga_fib_reject\n"
    "amiga_fib_forward:\n"
    "movem.l (sp)+,d0/a0\n"
    "rts\n"
    "amiga_fib_reject:\n"
    "addq.l #1,_amiga_fib_rejected\n"
    "movem.l (sp)+,d0/a0\n"
    "addq.l #4,sp\n"
    "moveq #0,d0\n"
    "rts\n"
    /* 68k's C stack needs word alignment. Select its residue modulo four
       before the call; D2 is callee-saved and retains the adjustment. */
    ".globl _amiga_fib_invoke\n"
    "_amiga_fib_invoke:\n"
    "move.l d2,-(sp)\n"
    "movea.l 8(sp),a0\n"
    "move.l sp,d2\n"
    "sub.l 12(sp),d2\n"
    "andi.l #3,d2\n"
    "suba.l d2,sp\n"
    "jsr (a0)\n"
    "adda.l d2,sp\n"
    "move.l (sp)+,d2\n"
    "rts\n"
);

static const char payload[] = "native file-info fixture";

static int exercise_file_queries(void)
{
    struct stat information;
    FILE *stream = NULL;
    char bytes[sizeof(payload)];
    int result = 0;
    int attributes;
    BPTR lock;
    FBSTRING specification = { "fixture.bin", 11, 11 };
    FBSTRING *name;

    if (stat("fixture.bin", &information) != 0 ||
        information.st_size != sizeof(payload) - 1) return 51;
    if (lstat("fixture.bin", &information) != 0 ||
        information.st_size != sizeof(payload) - 1) return 52;
    stream = fopen("fixture.bin", "rb");
    if (stream == NULL) return 53;
    if (fstat(fileno(stream), &information) != 0 ||
        information.st_size != sizeof(payload) - 1) { result = 54; goto done; }
    if (fseek(stream, 0, SEEK_END) != 0 || ftell(stream) != sizeof(payload) - 1 ||
        fseek(stream, 0, SEEK_SET) != 0) { result = 55; goto done; }
    if (fread(bytes, 1, sizeof(payload) - 1, stream) != sizeof(payload) - 1 ||
        memcmp(bytes, payload, sizeof(payload) - 1) != 0) { result = 56; goto done; }
    if (fclose(stream) != 0) return 57;
    stream = NULL;
    attributes = fb_FileGetAttr("fixture.bin");
    if (attributes < 0 || (attributes & FB_FILE_ATTR_DIRECTORY)) return 58;
    if (fb_FileSetAttr("fixture.bin", FB_FILE_ATTR_READONLY) != 0) return 59;
    attributes = fb_FileGetAttr("fixture.bin");
    if (attributes < 0 || !(attributes & FB_FILE_ATTR_READONLY)) return 60;
    if (fb_FileSetAttr("fixture.bin", 0) != 0) return 61;
    if (!(fb_FileDateTime("fixture.bin") > 0.0)) return 62;
    if (fb_FileCopy("fixture.bin", "copy.bin") != 0) return 63;
    stream = fopen("copy.bin", "rb");
    if (stream == NULL) return 69;
    if (fread(bytes, 1, sizeof(payload) - 1, stream) != sizeof(payload) - 1 ||
        memcmp(bytes, payload, sizeof(payload) - 1) != 0) { result = 70; goto done; }
    if (fclose(stream) != 0) return 57;
    stream = NULL;
    name = fb_Dir(&specification, FB_FILE_ATTR_ARCHIVE, &attributes);
    if (name == NULL || FB_STRSIZE(name) != 11 || strcmp(name->data, "fixture.bin") != 0)
        result = 64;
    if (name != NULL) fb_hStrDelTemp(name);
    if (result != 0) return result;
    if (chdir("subdir") != 0) return 65;
    if (chdir("/") != 0) return 66;
    lock = CreateDir("emptydir");
    if (lock == BNULL) return 67;
    UnLock(lock);
    if (rmdir("emptydir") != 0) return 68;
done:
    if (stream != NULL) fclose(stream);
    return result;
}

int amiga_test_fileinfo_alignment(void)
{
    BPTR directory, previous, stream, child;
    int ordinary = -1, word_aligned = -1;

    directory = CreateDir("fib-alignment-fixture");
    if (directory == BNULL) return 71;
    previous = CurrentDir(directory);
    child = CreateDir("subdir");
    if (child != BNULL) UnLock(child);
    stream = Open("fixture.bin", MODE_NEWFILE);
    if (stream == BNULL || child == BNULL) {
        if (stream != BNULL) Close(stream);
        ordinary = 72;
        goto cleanup;
    }
    if (Write(stream, (APTR)payload, sizeof(payload) - 1) != sizeof(payload) - 1) {
        Close(stream);
        ordinary = 73;
        goto cleanup;
    }
    Close(stream);
    amiga_fib_probe_task = FindTask(NULL);
    amiga_fib_calls = amiga_fib_rejected = 0;
    /* Each vector patch is temporary and confined to this private guest.
       Forbid prevents task switches while installing/restoring the set.
       NDK DOS offsets are -102 Examine, -390 ExamineFH, and -108 ExNext. */
    Forbid();
    amiga_fib_old_examine = SetFunction((struct Library *)DOSBase, -102, amiga_fib_examine);
    amiga_fib_old_examine_fh = SetFunction((struct Library *)DOSBase, -390, amiga_fib_examine_fh);
    amiga_fib_old_next = SetFunction((struct Library *)DOSBase, -108, amiga_fib_next);
    Permit();
    ordinary = amiga_fib_invoke(exercise_file_queries, 0);
    if (ordinary == 0) word_aligned = amiga_fib_invoke(exercise_file_queries, 2);
    Forbid();
    SetFunction((struct Library *)DOSBase, -102, (ULONG (*)())amiga_fib_old_examine);
    SetFunction((struct Library *)DOSBase, -390, (ULONG (*)())amiga_fib_old_examine_fh);
    SetFunction((struct Library *)DOSBase, -108, (ULONG (*)())amiga_fib_old_next);
    Permit();
    printf("FIB queries: %lu; rejected: %lu; aligned: %d; word: %d\n",
           amiga_fib_calls, amiga_fib_rejected, ordinary, word_aligned);
cleanup:
    /* CurrentDir transfers the active lock. chdir may have replaced and
       released the first one; close the returned current lock exactly once. */
    directory = CurrentDir(previous);
    if (directory != BNULL) UnLock(directory);
    SetProtection("fib-alignment-fixture/fixture.bin", 0);
    DeleteFile("fib-alignment-fixture/fixture.bin");
    DeleteFile("fib-alignment-fixture/copy.bin");
    DeleteFile("fib-alignment-fixture/subdir");
    DeleteFile("fib-alignment-fixture/emptydir");
    DeleteFile("fib-alignment-fixture");
    if (amiga_fib_rejected != 0) return 83;
    if (ordinary != 0) return ordinary;
    if (word_aligned != 0) return word_aligned;
    return amiga_fib_calls == 0 ? 84 : 0;
}

/* end of file-info-bridge.c */
