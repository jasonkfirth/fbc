/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_gosub.c

    Purpose:

        Provide the runtime stack used by generated C code for GOSUB and
        RETURN on NuttX.

    Responsibilities:

        - allocate one jump buffer for each active GOSUB
        - return the jump buffer address to the generated setjmp() call
        - longjmp() back to the saved return point
        - clean up pending GOSUB frames when a procedure exits

    This file intentionally does NOT contain:

        - parser support for GOSUB or RETURN
        - general ON ERROR state
        - platform-specific setjmp assembly
        - task or thread switching logic
*/

#include "setjmp.h"

#if !defined(FB_NUTTX_USE_GENERIC_GOSUB) || \
    (FB_NUTTX_USE_GENERIC_GOSUB == 0)

/* ------------------------------------------------------------------------- */
/* GOSUB stack                                                               */
/* ------------------------------------------------------------------------- */

typedef struct FB_NUTTX_GOSUB_NODE {
    jmp_buf buffer;
    struct FB_NUTTX_GOSUB_NODE *next;
} FB_NUTTX_GOSUB_NODE;

typedef struct FB_NUTTX_GOSUB_CTX {
    FB_NUTTX_GOSUB_NODE *top;
} FB_NUTTX_GOSUB_CTX;

void *fb_GosubPush(FB_NUTTX_GOSUB_CTX *ctx)
{
    FB_NUTTX_GOSUB_NODE *node;

    if (ctx == NULL)
        return NULL;

    node = (FB_NUTTX_GOSUB_NODE *)malloc(sizeof(FB_NUTTX_GOSUB_NODE));

    if (node == NULL)
        return NULL;

    node->next = ctx->top;
    ctx->top = node;

    return (void *)&ctx->top->buffer;
}

int32 fb_GosubPop(FB_NUTTX_GOSUB_CTX *ctx)
{
    FB_NUTTX_GOSUB_NODE *node;

    if ((ctx == NULL) || (ctx->top == NULL))
        return -1;

    node = ctx->top;
    ctx->top = node->next;
    free(node);

    return 0;
}

int32 fb_GosubReturn(FB_NUTTX_GOSUB_CTX *ctx)
{
    FB_NUTTX_GOSUB_NODE *node;
    jmp_buf buffer;

    if ((ctx == NULL) || (ctx->top == NULL))
        return -1;

    node = ctx->top;
    ctx->top = node->next;

    /*
        The saved jump buffer lives in a heap node that must be released
        before control leaves this function.  Copying it to a stack-local
        jmp_buf mirrors the normal rtlib approach and keeps the heap stack
        balanced even when longjmp() never returns here.
    */
    memset(&buffer, 0, sizeof(buffer));
    memcpy(buffer, node->buffer, sizeof(buffer));
    free(node);

    longjmp(buffer, -1);
}

void fb_GosubExit(FB_NUTTX_GOSUB_CTX *ctx)
{
    while ((ctx != NULL) && (ctx->top != NULL)) {
        FB_NUTTX_GOSUB_NODE *node = ctx->top;
        ctx->top = node->next;
        free(node);
    }
}

#endif

/* end of fb_nuttx_gosub.c */
