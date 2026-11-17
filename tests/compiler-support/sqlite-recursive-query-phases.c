/*
    Project: SQLite FreeBASIC port qualification
    File: sqlite-recursive-query-phases.c
    Exercise complete emitted recursive-query generation with canonical types.
    Controlled VDBE dependencies check SELECT links and cleanup ownership.
    SQL parsing and virtual-machine execution have separate coverage.
*/
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Each serial case owns a finite SELECT chain and its borrowed expression lists. */
static struct fb_type_18SQLITESELECTRECORD terms[34];
static struct fb_type_18SQLITEPARSECONTEXT context;
static struct fb_type_7SQLITE3 database;
static struct fb_type_19SQLITESRCLISTRECORD sources;
static struct fb_type_20SQLITEEXPRLISTRECORD columns, ordering;
static struct fb_type_10SQLITEEXPR limit_expr;
static struct fb_type_16SQLITESELECTDEST destination;
static struct fb_type_24SQLITE3VDBEKEYINFOPREFIX key_info;
static struct fb_type_14SQLITE3COLLSEQ collation;
static struct fb_type_18SQLITESELECTRECORD *anchor;
static int vdbe_token;
static unsigned operations, selects, deletes, errors;
static unsigned long long transcript;
static int deny_authorization, fail_setup, fail_allocation;
static const char *last_error;

static void record(int code, int a, int b, int c)
{
    /* Unsigned wrap is intentional so transcripts are identical across tools. */
    transcript = transcript * 131ULL + (unsigned)code;
    transcript = transcript * 131ULL + (unsigned)a;
    transcript = transcript * 131ULL + (unsigned)b;
    transcript = transcript * 131ULL + (unsigned)c;
    operations++;
}
void sqlite3ErrorMsg(fb_c_pointer_view_1 *parse, const char *message, ...)
{
    assert(parse == &context && message != NULL);
    last_error = message;
    errors++;
    context.NERR++;
}
void sqlite3ExprListDelete(fb_c_pointer_view_18 *db, fb_c_pointer_view_10 *list)
{
    assert(db == &database && list == NULL);
    deletes++;
}
int32 sqlite3Select(fb_c_pointer_view_1 *parse, fb_c_pointer_view_2 *select, fb_c_pointer_view_14 *dest)
{
    assert(parse == &context && dest != NULL);
    assert(terms[0].PORDERBY == NULL && terms[0].PLIMIT == NULL);
    if (selects == 0) {
        assert(select == anchor && anchor != NULL && anchor->PNEXT == NULL);
    } else {
        struct fb_type_18SQLITESELECTRECORD *term = &terms[0];
        assert(select == &terms[0] && anchor->PNEXT == &terms[0]);
        while (term->PPRIOR != NULL) term = term->PPRIOR;
        assert(term != anchor);
    }
    selects++;
    return fail_setup && selects == 1 ? 1 : 0;
}
int32 sqlite3VdbeAddOp1(fb_c_pointer_view_5 *vm, int32 op, int32 a)
{
    assert((void *)vm == &vdbe_token);
    record(op, a, 0, 0);
    return (int32)operations;
}
int32 sqlite3VdbeAddOp2(fb_c_pointer_view_5 *vm, int32 op, int32 a, int32 b)
{
    assert((void *)vm == &vdbe_token);
    record(op, a, b, 0);
    return (int32)operations;
}
int32 sqlite3VdbeAddOp3(fb_c_pointer_view_5 *vm, int32 op, int32 a, int32 b, int32 c)
{
    assert((void *)vm == &vdbe_token);
    record(op, a, b, c);
    return (int32)operations;
}
int32 sqlite3VdbeAddOp4(fb_c_pointer_view_5 *vm, int32 op, int32 a, int32 b, int32 c, const char *data, int32 kind)
{
    assert((void *)vm == &vdbe_token && kind == -9 && (const void *)data == &key_info);
    record(op, a, b, c);
    return (int32)operations;
}
int32 sqlite3VdbeGoto(fb_c_pointer_view_5 *vm, int32 target)
{
    assert((void *)vm == &vdbe_token);
    record(900, target, 0, 0);
    return (int32)operations;
}
int32 sqlite3VdbeExplainText(fb_c_pointer_view_1 *parse, uint8 indent, const char *text)
{
    assert(parse == &context && indent == 1 && text != NULL);
    record(901, strcmp(text, "SETUP") == 0, 0, 0);
    return 0;
}
int32 sqlite3VdbeMakeLabel(void *parse)
{
    assert(parse == &context);
    record(902, 0, 0, 0);
    return -(int32)operations;
}
void sqlite3VdbeResolveLabel(fb_c_pointer_view_5 *vm, int32 label)
{
    assert((void *)vm == &vdbe_token && label < 0);
    record(903, label, 0, 0);
}
int32 sqlite3AuthCheck(void *parse, int32 code, const char *a, const char *b, const char *c)
{
    assert(parse == &context && code == 33 && a == NULL && b == NULL && c == NULL);
    return deny_authorization;
}
void *sqlite3KeyInfoAlloc(void *db, int32 fields, int32 extra)
{
    assert(db == &database && fields == 1 && extra == 1);
    return fail_allocation ? NULL : &key_info;
}
void sqlite3SelectDestInit(void *data, int32 kind, int32 cursor_id)
{
    struct fb_type_16SQLITESELECTDEST *dest = data;
    memset(dest, 0, sizeof(*dest));
    assert(kind >= 3 && kind <= 6);
    dest->EDEST = (uint8)kind;
    dest->ISDPARM = cursor_id;
}
static void codeOffset(fb_c_pointer_view_5 *vm, int32 offset, int32 label)
{
    assert((void *)vm == &vdbe_token);
    record(904, offset, label, 0);
}
static void selectInnerLoop(fb_c_pointer_view_1 *parse, fb_c_pointer_view_2 *select, int32 cursor_id,
                            fb_c_pointer_view_4 *sort, fb_c_pointer_view_13 *distinct,
                            fb_c_pointer_view_14 *dest, int32 next, int32 end)
{
    assert(parse == &context && select == &terms[0] && sort == NULL && distinct == NULL && dest == &destination);
    record(905, cursor_id, next, end);
}
static void computeLimitRegisters(fb_c_pointer_view_1 *parse, fb_c_pointer_view_2 *select, int32 label)
{
    assert(parse == &context && select == &terms[0] && label < 0);
}
static fb_c_pointer_view_11 *multiSelectCollSeq(fb_c_pointer_view_1 *parse, fb_c_pointer_view_2 *select, int32 column)
{
    assert(parse == &context && select == &terms[0] && column == 0);
    return NULL;
}
static fb_c_pointer_view_8 *multiSelectByMergeKeyInfo(fb_c_pointer_view_1 *parse, fb_c_pointer_view_2 *select, int32 extra)
{
    assert(parse == &context && select == &terms[0] && extra == 1);
    return &key_info;
}

int main(void)
{
    unsigned test;
    unsigned total = 40;
#ifdef TEST_MISSING_ANCHOR
    total += 4;
#endif
    for (test = 0; test < total; test++) {
        unsigned i, length = 1 + test % 8;
        int rejected = 0;
        memset(terms, 0, sizeof(terms));
        memset(&context, 0, sizeof(context));
        memset(&database, 0, sizeof(database));
        memset(&sources, 0, sizeof(sources));
        memset(&ordering, 0, sizeof(ordering));
        memset(&destination, 0, sizeof(destination));
        memset(&key_info, 0, sizeof(key_info));
        operations = selects = deletes = errors = 0;
        transcript = 0;
        last_error = "";
        deny_authorization = fail_setup = fail_allocation = 0;
        if (test == 39) length = 32;
        anchor = &terms[length];
        for (i = 0; i < length; i++) {
            terms[i].SELFLAGS = 0x2000;
            terms[i].OP = 136;
            terms[i].PPRIOR = &terms[i + 1];
        }
        anchor->PNEXT = &terms[0];
        context.DB = &database;
        context.PVDBE = &vdbe_token;
        database.PDFLTCOLL = &collation;
        columns.NEXPR = ordering.NEXPR = 1;
        sources.NSRC = 1;
        sources.A[0].FG.FLAGS = 128;
        sources.A[0].ICURSOR = 3;
        terms[0].PSRC = &sources;
        terms[0].PELIST = &columns;
        terms[0].PLIMIT = &limit_expr;
        terms[0].PORDERBY = test & 1 ? &ordering : NULL;
        terms[0].ILIMIT = test & 2 ? 7 : 0;
        terms[0].IOFFSET = test & 4 ? 8 : 0;
        terms[0].OP = test & 8 ? 135 : 136;
        if (test >= 16 && test < 24) fail_setup = 1;
        if (test >= 24 && test < 32) { terms[length / 2].SELFLAGS |= 8; rejected = 1; }
        if (test == 32 || test == 33) deny_authorization = 1;
        if (test == 34 || test == 35) terms[0].PWIN = &ordering;
        if (test == 36 || test == 37) { terms[0].OP = 135; fail_allocation = 1; }
        if (test >= 40) { terms[length - 1].PPRIOR = NULL; rejected = 1; }
        generateWithRecursiveQuery(&context, &terms[0], &destination);
        assert(terms[0].PLIMIT == &limit_expr);
        assert(terms[0].PORDERBY == (test & 1 ? &ordering : NULL));
        assert(anchor->PNEXT == &terms[0]);
        for (i = 0; i < length; i++) {
            assert(terms[i].PPRIOR == (test >= 40 && i == length - 1 ? NULL : &terms[i + 1]));
        }
        if (rejected) assert(errors == 1 && selects == 0 && deletes == 1);
        if (test >= 40) assert(strcmp(last_error, "recursive query has no non-recursive term") == 0);
        if (test < 16 || test == 36 || test == 37 || (test >= 38 && test < 40)) assert(selects == 2);
        printf("CASE %u calls=%u deletes=%u errors=%u ops=%u digest=%llu tabs=%d mem=%d error=%s\n",
               test, selects, deletes, errors, operations, transcript, context.NTAB, context.NMEM, last_error);
    }
    return 0;
}
/* end of sqlite-recursive-query-phases.c */
