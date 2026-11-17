/*
    Project: SQLite FreeBASIC port qualification
    File: sqlite-btree-insertion-phases.c
    Exercise the complete emitted sqlite3BtreeInsert body with canonical types.
    Controlled page and cursor helpers record phase order and inject errors.
    Pager storage and whole-engine SQL execution have separate coverage.
*/
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* One serial case owns these records. Helpers never retain their addresses. */
static struct fb_type_8BTCURSOR cursor;
static struct fb_type_8BTSHARED shared;
static struct fb_type_5BTREE tree;
static struct fb_type_7MEMPAGE page;
static struct fb_type_18SQLITEBTREEPAYLOAD payload;
static uint8 scratch[64];
static uint8 old_cell[8];
static int trace[64];
static unsigned trace_count;
static int fail_event;
static unsigned fail_visit;
static unsigned event_visits[24];
static int null_cell;
static int parsed_overflow;
static uint16 formatted_size;

/* SQLite's stable public result codes, independent of either tested body. */
enum {
    TEST_OK = 0, TEST_BUSY = 5, TEST_NOMEM = 7, TEST_READONLY = 8,
    TEST_CORRUPT = 11, TEST_TOOBIG = 18, TEST_MISUSE = 21
};
static const int32 expected_results[] = {
    TEST_OK, TEST_OK, TEST_OK, TEST_BUSY, TEST_BUSY, TEST_BUSY, TEST_BUSY, TEST_BUSY,
    TEST_BUSY, TEST_BUSY, TEST_READONLY, TEST_CORRUPT, TEST_CORRUPT, TEST_CORRUPT,
    TEST_CORRUPT, TEST_OK, TEST_BUSY, TEST_NOMEM, TEST_BUSY, TEST_OK, TEST_BUSY,
    TEST_BUSY, TEST_CORRUPT, TEST_CORRUPT, TEST_OK, TEST_CORRUPT, TEST_OK,
    TEST_CORRUPT, TEST_BUSY, TEST_OK, TEST_BUSY, TEST_OK, TEST_BUSY, TEST_OK,
    TEST_CORRUPT, TEST_CORRUPT, TEST_OK, TEST_CORRUPT, TEST_OK, TEST_BUSY,
    TEST_CORRUPT, TEST_OK, TEST_MISUSE, TEST_MISUSE, TEST_READONLY, TEST_READONLY,
    TEST_MISUSE, TEST_MISUSE, TEST_MISUSE, TEST_TOOBIG, TEST_CORRUPT, TEST_OK,
    TEST_OK, TEST_BUSY, TEST_BUSY, TEST_OK
};

static int event(int id)
{
    assert(id > 0 && id < 24);
    assert(trace_count < sizeof(trace) / sizeof(trace[0]));
    trace[trace_count++] = id;
    event_visits[id]++;
    return fail_event == id && event_visits[id] == fail_visit ? 5 : 0;
}

static int32 saveAllCursors(fb_c_pointer_view_4 *bt, uint32 root, fb_c_pointer_view_7 *cur)
{
    assert(bt == &shared && root == 1 && cur == &cursor);
    return event(1);
}
static int32 BTREECURSORRESTOREINTERNAL(fb_c_pointer_view_7 *cur, fb_c_pointer_view_9 *location)
{
    assert(cur == &cursor && location != NULL);
    *location = 0;
    return event(2);
}
static void invalidateIncrblobCursors(fb_c_pointer_view_3 *bt, uint32 root, int64 key, int32 clear)
{
    assert(bt == &tree && root == 1 && key == payload.NKEY && clear == 0);
    (void)event(3);
}
static int32 BTREETABLEMOVETOINTERNAL(fb_c_pointer_view_7 *cur, int64 key, int32 append, fb_c_pointer_view_9 *location)
{
    assert(cur == &cursor && key == payload.NKEY && (append == 0 || append == 1));
    assert(location != NULL);
    *location = 0;
    return event(4);
}
static int32 BTREEINDEXMOVETOPAYLOAD(fb_c_pointer_view_7 *cur, fb_c_pointer_view_14 *data, fb_c_pointer_view_9 *location)
{
    assert(cur == &cursor && data == &payload && location != NULL);
    *location = 0;
    return event(5);
}
static int32 getCellInfo(fb_c_pointer_view_7 *cur)
{
    assert(cur == &cursor);
    return event(6);
}
static int32 BTREEOVERWRITECELL(fb_c_pointer_view_7 *cur, fb_c_pointer_view_14 *data)
{
    assert(cur == &cursor);
    if (cur->PKEYINFO == NULL) {
        assert(data == &payload);
    } else {
        assert(data->PKEY == NULL && data->NKEY == 0 && data->PDATA == payload.PKEY);
        assert(data->AMEM == NULL && data->NMEM == 0 && data->NDATA == payload.NKEY && data->NZERO == 0);
    }
    return event(7);
}
int32 sqlite3BtreeComputeFreeSpace(fb_c_pointer_view_2 *pg)
{
    assert(pg == &page);
    pg->NFREE = 32;
    return event(8);
}
uint16 sqlite3BtreeCellSize(fb_c_pointer_view_2 *pg, uint8 *cell)
{
    assert(pg == &page && cell == scratch);
    (void)event(9);
    return formatted_size;
}
static void parse_cell(void *pg, uint8 *cell, void *output)
{
    struct fb_type_8CELLINFO *info = output;
    assert(pg == &page && cell == scratch);
    memset(info, 0, sizeof(*info));
    info->NPAYLOAD = 4;
    info->NLOCAL = parsed_overflow ? 1 : 4;
    (void)event(10);
}
static void PTRMAPPUT(fb_c_pointer_view_4 *bt, uint32 overflow, uint8 kind, uint32 parent, fb_c_pointer_view_9 *rc)
{
    assert(bt == &shared && overflow == 9 && kind == 3 && parent == page.PGNO && rc != NULL);
    *rc = event(11);
}
static int32 BTREEBUILDCELL(fb_c_pointer_view_2 *pg, fb_c_pointer_view_14 *data, uint8 *cell, int32 capacity, fb_c_pointer_view_9 *size)
{
    assert(pg == &page && data == &payload && cell == scratch && capacity == 64 && size != NULL);
    *size = 8;
    return event(12);
}
static uint8 *BTREECURSORCELL(fb_c_pointer_view_2 *pg, int32 index)
{
    assert(pg == &page && index == cursor.IX);
    (void)event(13);
    return null_cell ? NULL : old_cell;
}
static int32 clearCellOverflow(fb_c_pointer_view_2 *pg, const uint8 *cell, fb_c_pointer_view_11 *info)
{
    assert(pg == &page && cell == old_cell && info == &cursor.INFO);
    return event(14);
}
int32 sqlite3BtreeDropCell(fb_c_pointer_view_2 *pg, int32 index)
{
    assert(pg == &page && index == cursor.IX);
    return event(15);
}
static void BTREEMOVETOOVERFLOWSTART(fb_c_pointer_view_7 *cur)
{
    assert(cur == &cursor);
    (void)event(16);
}
static int32 BTREEINSERTCELLFAST(fb_c_pointer_view_2 *pg, int32 index, const uint8 *cell, int32 size)
{
    assert(pg == &page && index == cursor.IX && cell == scratch && size >= 4);
    return event(17);
}
static int32 BTREEINSERTCELLWITHCHILD(fb_c_pointer_view_2 *pg, int32 index, uint8 *cell, int32 size, uint32 child)
{
    assert(pg == &page && index == cursor.IX && cell == scratch && size == 8 && child == 9);
    return event(18);
}
static int32 balance(fb_c_pointer_view_7 *cur)
{
    assert(cur == &cursor);
    return event(19);
}
static void btreeReleaseAllCursorPages(fb_c_pointer_view_7 *cur)
{
    assert(cur == &cursor && page.NOVERFLOW == 0);
    cur->PPAGE = NULL;
    (void)event(20);
}
static int32 BTREESETCURSORREQUIRESEEK(fb_c_pointer_view_7 *cur, fb_c_pointer_view_14 *data)
{
    assert(cur == &cursor && data == &payload);
    cur->ESTATE = 3;
    return event(21);
}
static uint32 BTREEREADU32(const uint8 *bytes)
{
    assert(bytes == old_cell || bytes == scratch + 4);
    (void)event(22);
    return 9;
}

static void reset_case(void)
{
    memset(&cursor, 0, sizeof(cursor));
    memset(&shared, 0, sizeof(shared));
    memset(&tree, 0, sizeof(tree));
    memset(&page, 0, sizeof(page));
    memset(&payload, 0, sizeof(payload));
    memset(scratch, 0, sizeof(scratch));
    memset(event_visits, 0, sizeof(event_visits));
    trace_count = 0;
    fail_event = 0;
    fail_visit = 1;
    null_cell = 0;
    parsed_overflow = 1;
    formatted_size = 8;
    cursor.PBT = &shared;
    cursor.PBTREE = &tree;
    cursor.PPAGE = &page;
    cursor.PGNOROOT = 1;
    cursor.CURFLAGS = 1 | 8;
    cursor.INFO.NKEY = 7;
    cursor.INFO.NPAYLOAD = 3;
    cursor.INFO.NSIZE = 8;
    tree.PBT = &shared;
    tree.INTRANS = 2;
    shared.PTMPSPACE = scratch;
    shared.PAGESIZE = 64;
    shared.NPREFORMATSIZE = 8;
    page.ISINIT = 1;
    page.INTKEY = 1;
    page.LEAF = 1;
    page.NFREE = 32;
    page.NCELL = 1;
    page.PGNO = 2;
    page.MAXLOCAL = 2;
    page.XPARSECELL = parse_cell;
    payload.NKEY = 7;
    payload.NDATA = 3;
    payload.PDATA = old_cell;
}

int main(void)
{
    unsigned test;
    for (test = 0; test < 56; test++) {
        int32 flags = 0, location = 1, rc;
        unsigned index;
        reset_case();
        switch (test) {
        case 1: flags = 2; location = 0; break;
        case 2: case 53:
            cursor.PKEYINFO = &tree; page.INTKEY = 0;
            payload.PKEY = old_cell; payload.NKEY = 3;
            location = 0; if (test == 53) fail_event = 7; break;
        case 3: flags = 2; location = 0; fail_event = 6; break;
        case 4: flags = 2; location = 0; fail_event = 7; break;
        case 5: location = 0; fail_event = 4; break;
        case 6:
            cursor.PKEYINFO = &tree; page.INTKEY = 0;
            payload.PKEY = old_cell; location = 0; fail_event = 5; break;
        case 7: cursor.CURFLAGS |= 32; fail_event = 1; break;
        case 8: cursor.ESTATE = 3; fail_event = 2; break;
        case 9: cursor.ESTATE = 4; cursor.SKIPNEXT = 5; break;
        case 10: tree.INTRANS = 0; break;
        case 11: cursor.PPAGE = NULL; break;
        case 12: cursor.IPAGE = -1; break;
        case 13: page.ISINIT = 0; break;
        case 14: page.NFREE = -2; break;
        case 15: page.NFREE = -1; break;
        case 16: page.NFREE = -1; fail_event = 8; break;
        case 17: shared.PTMPSPACE = NULL; break;
        case 18: fail_event = 12; break;
        case 19: case 20: case 21: case 22: case 23: case 26: case 54:
            flags = 2; location = 0; cursor.INFO.NKEY = 8;
            if (test == 20) fail_event = 14;
            if (test == 21) fail_event = 15;
            if (test == 22) null_cell = 1;
            if (test == 23) cursor.IX = 1;
            if (test == 26) page.LEAF = 0;
            if (test == 54) { fail_event = 6; fail_visit = 2; }
            break;
        case 24: location = -1; break;
        case 25: location = -1; page.LEAF = 0; break;
        case 27: page.LEAF = 0; break;
        case 28: fail_event = 17; break;
        case 29: case 30: case 31: case 32: case 55:
            page.NOVERFLOW = 1; cursor.CURFLAGS |= 6;
            if (test == 30) fail_event = 19;
            if (test == 31 || test == 32) flags = 2;
            if (test == 32) fail_event = 21;
            break;
        case 33: case 34: case 35: case 36: case 37: case 38: case 39: case 40: case 41:
            flags = 128;
            if (test == 34) shared.NPREFORMATSIZE = -1;
            if (test == 35) shared.NPREFORMATSIZE = 65;
            if (test == 36) { shared.NPREFORMATSIZE = 2; formatted_size = 4; }
            if (test == 37) formatted_size = 7;
            if (test >= 38) shared.AUTOVACUUM = 1;
            if (test == 39) fail_event = 11;
            if (test == 40) page.XPARSECELL = NULL;
            if (test == 41) parsed_overflow = 0;
            break;
        case 43: flags = 1; break;
        case 44: shared.BTSFLAGS = 1; break;
        case 45: tree.PBT = NULL; break;
        case 46: payload.NDATA = -1; break;
        case 47: cursor.PKEYINFO = &tree; break;
        case 48: cursor.PKEYINFO = &tree; payload.PKEY = old_cell; payload.NKEY = 2147483648LL; break;
        case 49: payload.NDATA = 2147483647; payload.NZERO = 1; break;
        case 50: cursor.CURFLAGS |= 32; cursor.IPAGE = -1; break;
        case 51: tree.HASINCRBLOBCUR = 1; flags = 2; location = 0; break;
        case 52: cursor.ESTATE = 1; break;
        default: break;
        }
        rc = sqlite3BtreeInsert(&cursor, test == 42 ? NULL : &payload, flags, location);
        assert(test < sizeof(expected_results) / sizeof(expected_results[0]));
        assert(rc == expected_results[test]);
        printf("CASE %u rc=%d state=%u flags=%u ix=%u size=%u free=%d overflow=%u page=%d trace=",
               test, rc, cursor.ESTATE, cursor.CURFLAGS, cursor.IX, cursor.INFO.NSIZE,
               page.NFREE, page.NOVERFLOW, cursor.PPAGE != NULL);
        for (index = 0; index < trace_count; index++) printf("%d,", trace[index]);
        puts("");
        if (fail_event != 0 && event_visits[fail_event] >= fail_visit) assert(rc == 5);
        if (test == 1 || test == 2 || test == 4 || test == 51 || test == 53) {
            assert(event_visits[7] == 1 && event_visits[12] == 0 && event_visits[17] == 0);
        }
    }
    return 0;
}
/* end of sqlite-btree-insertion-phases.c */
