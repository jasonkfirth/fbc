/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/pipe.c

    Purpose:
        Connect command input or output to a bounded native byte stream.

    Responsibilities:
        - serve real DOS READ, WRITE, and END packets for two file handles
        - run the command in a separate shell context
        - apply backpressure and deliver EOF when the writer closes
        - close endpoints and join both workers before releasing their state

    This file intentionally does NOT contain:
        - command emulation, temporary-file capture, or BASIC file-number policy

    A private handler process owns the ring and both pending-packet lists.
    Packet buffers remain owned by their waiting DOS callers until ReplyPkt.
    A full ring leaves writes pending; an empty ring leaves reads pending.
    Closing the reader rejects pending writes, while closing the writer lets
    the reader drain buffered bytes before receiving EOF.

    DOS packet layout used here:
        READ/WRITE: Arg1 = endpoint cookie, Arg2 = byte buffer, Arg3 = length
        END:        Arg1 = endpoint cookie
        Res1 = bytes transferred (or END success), Res2 = native error

    The handler's standard process message port must also be its file-handle
    port because ReplyPkt publishes that port for subsequent requests. Only
    the handler changes ring state. Startup publishes its port before signalling
    the parent; pclose joins the command and handler before freeing the context.
*/

#include "../fb.h"

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <fcntl.h>
#include <pthread.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <inline/alib.h>

#define AMIGA_PIPE_BYTES 32768U
#define AMIGA_PIPE_COMMAND_BYTES 65535U

typedef struct AMIGA_PIPE AMIGA_PIPE;
typedef struct AMIGA_PIPE_ENDPOINT {
    AMIGA_PIPE *pipe;
    int writer;
} AMIGA_PIPE_ENDPOINT;

struct AMIGA_PIPE {
    AMIGA_PIPE *next;
    FILE *stream;
    char *command;
    struct Task *parent;
    LONG ready_signal;
    struct MsgPort *port;
    pthread_t server, child;
    BPTR reader_handle, writer_handle, output;
    LONG result;
    AMIGA_PIPE_ENDPOINT reader, writer;
    struct List reads, writes;
    unsigned int head, used;
    int reader_open, writer_open;
    int input;
    unsigned char bytes[AMIGA_PIPE_BYTES];
};

/* The BASIC runtime lock serializes registry access. Command and handler
   workers never enter it, so blocking pipe I/O cannot prevent their progress. */
static AMIGA_PIPE *pipes;
int fb_hAmigaRegisterHandle(unsigned long handle, int flags);

static struct DosPacket *pending_packet(struct List *list)
{
    struct Message *message = (struct Message *)list->lh_Head;
    if (message->mn_Node.ln_Succ == NULL) return NULL;
    return (struct DosPacket *)message->mn_Node.ln_Name;
}

static void finish_packet(struct DosPacket *packet, LONG result, LONG error)
{
    Remove(&packet->dp_Link->mn_Node);
    ReplyPkt(packet, result, error);
}

static void transfer_packets(AMIGA_PIPE *pipe)
{
    int changed;
    do {
        struct DosPacket *packet;
        changed = FALSE;
        packet = pending_packet(&pipe->reads);
        if (packet != NULL && (!pipe->reader_open || pipe->used != 0 || !pipe->writer_open)) {
            unsigned int length = (unsigned int)packet->dp_Arg3;
            unsigned char *destination = (unsigned char *)packet->dp_Arg2;
            unsigned int i;
            if (length > pipe->used) length = pipe->used;
            if (!pipe->reader_open) finish_packet(packet, -1, ERROR_INVALID_LOCK);
            else {
                for (i = 0; i < length; ++i) {
                    destination[i] = pipe->bytes[pipe->head];
                    if (++pipe->head == AMIGA_PIPE_BYTES) pipe->head = 0;
                }
                pipe->used -= length;
                finish_packet(packet, (LONG)length, 0);
            }
            changed = TRUE;
        }
        packet = pending_packet(&pipe->writes);
        if (packet != NULL && (!pipe->reader_open || !pipe->writer_open || pipe->used < AMIGA_PIPE_BYTES)) {
            unsigned int length = (unsigned int)packet->dp_Arg3;
            const unsigned char *source = (const unsigned char *)packet->dp_Arg2;
            unsigned int tail = pipe->head + pipe->used;
            unsigned int i;
            if (tail >= AMIGA_PIPE_BYTES) tail -= AMIGA_PIPE_BYTES;
            if (length > AMIGA_PIPE_BYTES - pipe->used) length = AMIGA_PIPE_BYTES - pipe->used;
            if (!pipe->reader_open || !pipe->writer_open)
                finish_packet(packet, -1, ERROR_OBJECT_WRONG_TYPE);
            else {
                for (i = 0; i < length; ++i) {
                    pipe->bytes[tail] = source[i];
                    if (++tail == AMIGA_PIPE_BYTES) tail = 0;
                }
                pipe->used += length;
                finish_packet(packet, (LONG)length, 0);
            }
            changed = TRUE;
        }
    } while (changed);
}

static void *pipe_server(void *argument)
{
    AMIGA_PIPE *pipe = argument;
    struct Process *process = (struct Process *)FindTask(NULL);
    NewList(&pipe->reads);
    NewList(&pipe->writes);
    pipe->port = &process->pr_MsgPort;
    Signal(pipe->parent, 1UL << pipe->ready_signal);

    while (pipe->reader_open || pipe->writer_open) {
        struct Message *message;
        WaitPort(pipe->port);
        while ((message = GetMsg(pipe->port)) != NULL) {
            struct DosPacket *packet = (struct DosPacket *)message->mn_Node.ln_Name;
            AMIGA_PIPE_ENDPOINT *endpoint = (AMIGA_PIPE_ENDPOINT *)packet->dp_Arg1;
            if (endpoint != &pipe->reader && endpoint != &pipe->writer) {
                ReplyPkt(packet, DOSFALSE, ERROR_INVALID_LOCK);
                continue;
            }
            if (packet->dp_Type == ACTION_END) {
                if (endpoint->writer) pipe->writer_open = FALSE;
                else pipe->reader_open = FALSE;
                ReplyPkt(packet, DOSTRUE, 0);
            } else if (packet->dp_Type == ACTION_READ || packet->dp_Type == ACTION_WRITE) {
                if (packet->dp_Arg3 < 0 || (packet->dp_Arg3 != 0 && packet->dp_Arg2 == 0) ||
                    (packet->dp_Type == ACTION_WRITE) != endpoint->writer) {
                    ReplyPkt(packet, -1, ERROR_OBJECT_WRONG_TYPE);
                } else if (packet->dp_Arg3 == 0) ReplyPkt(packet, 0, 0);
                else AddTail(packet->dp_Type == ACTION_READ ? &pipe->reads : &pipe->writes,
                             &message->mn_Node);
            } else if (packet->dp_Type == ACTION_FLUSH) ReplyPkt(packet, DOSTRUE, 0);
            else ReplyPkt(packet, -1, ERROR_ACTION_NOT_KNOWN);
            transfer_packets(pipe);
        }
    }
    transfer_packets(pipe);
    return pipe;
}

static BPTR make_endpoint(AMIGA_PIPE *pipe, AMIGA_PIPE_ENDPOINT *endpoint, LONG mode)
{
    struct TagItem tags[] = { { ADO_FH_Mode, (ULONG)mode }, { TAG_DONE, 0 } };
    struct FileHandle *handle = AllocDosObject(DOS_FILEHANDLE, tags);
    if (handle == NULL) return 0;
    handle->fh_Type = pipe->port;
    handle->fh_Arg1 = (LONG)endpoint;
    return MKBADDR(handle);
}

static void *pipe_command(void *argument)
{
    AMIGA_PIPE *pipe = argument;
    BPTR input = pipe->input ? Open((CONST_STRPTR)"NIL:", MODE_OLDFILE) : pipe->reader_handle;
    BPTR output = pipe->input ? pipe->writer_handle : pipe->output;
    struct TagItem tags[] = {
        { SYS_Input, (ULONG)input }, { SYS_Output, (ULONG)output },
        { SYS_Asynch, FALSE }, { TAG_DONE, 0 }
    };
    pipe->result = input != 0 ? SystemTagList((CONST_STRPTR)pipe->command, tags) : -1;
    if (pipe->input) {
        if (input != 0) Close(input);
        Close(pipe->writer_handle);
    } else Close(pipe->reader_handle);
    return pipe;
}

FILE *fb_hAmigaPipeOpen(const char *command, const char *mode)
{
    AMIGA_PIPE *pipe;
    size_t length;
    int descriptor;
    BPTR parent_handle;

    if (command == NULL || mode == NULL || (*mode != 'r' && *mode != 'w')) return NULL;
    length = strnlen(command, AMIGA_PIPE_COMMAND_BYTES + 1U);
    if (length == 0 || length > AMIGA_PIPE_COMMAND_BYTES) { errno = EINVAL; return NULL; }
    pipe = calloc(1, sizeof(*pipe));
    if (pipe == NULL) return NULL;
    pipe->command = malloc(length + 1);
    if (pipe->command == NULL) { free(pipe); return NULL; }
    memcpy(pipe->command, command, length + 1);
    pipe->input = *mode == 'r';
    pipe->reader.pipe = pipe->writer.pipe = pipe;
    pipe->writer.writer = TRUE;
    pipe->reader_open = pipe->writer_open = TRUE;
    pipe->parent = FindTask(NULL);
    pipe->output = Output();
    pipe->ready_signal = AllocSignal(-1);
    if (pipe->ready_signal < 0) goto fail;
    if (pthread_create(&pipe->server, NULL, pipe_server, pipe) != 0) {
        FreeSignal(pipe->ready_signal);
        goto fail;
    }
    Wait(1UL << pipe->ready_signal);
    FreeSignal(pipe->ready_signal);
    pipe->reader_handle = make_endpoint(pipe, &pipe->reader, MODE_OLDFILE);
    pipe->writer_handle = make_endpoint(pipe, &pipe->writer, MODE_NEWFILE);
    if (pipe->reader_handle == 0 || pipe->writer_handle == 0) goto fail_server;
    parent_handle = pipe->input ? pipe->reader_handle : pipe->writer_handle;
    descriptor = fb_hAmigaRegisterHandle((unsigned long)parent_handle, pipe->input ? O_RDONLY : O_WRONLY);
    if (descriptor < 0) goto fail_server;
    pipe->stream = fdopen(descriptor, mode);
    if (pipe->stream == NULL) {
        close(descriptor);
        if (pipe->input) pipe->reader_handle = 0;
        else pipe->writer_handle = 0;
        goto fail_server;
    }
    if (pthread_create(&pipe->child, NULL, pipe_command, pipe) != 0) {
        fclose(pipe->stream);
        if (pipe->input) pipe->reader_handle = 0;
        else pipe->writer_handle = 0;
        if (pipe->input) Close(pipe->writer_handle);
        else Close(pipe->reader_handle);
        pthread_join(pipe->server, NULL);
        goto fail;
    }
    pipe->next = pipes;
    pipes = pipe;
    return pipe->stream;

fail_server:
    /* A failed endpoint allocation still needs an END packet to wake the
       waiting handler. Send missing-endpoint ENDs directly through DOS. */
    if (pipe->reader_handle != 0) Close(pipe->reader_handle);
    else if (pipe->reader_open) DoPkt(pipe->port, ACTION_END, (LONG)&pipe->reader, 0, 0, 0, 0);
    if (pipe->writer_handle != 0) Close(pipe->writer_handle);
    else if (pipe->writer_open) DoPkt(pipe->port, ACTION_END, (LONG)&pipe->writer, 0, 0, 0, 0);
    pthread_join(pipe->server, NULL);
fail:
    free(pipe->command);
    free(pipe);
    return NULL;
}

int fb_hAmigaPipeClose(FILE *stream)
{
    AMIGA_PIPE **link, *pipe;
    for (link = &pipes; *link != NULL && (*link)->stream != stream; link = &(*link)->next) {}
    if (*link == NULL) { errno = EINVAL; return -1; }
    pipe = *link;
    *link = pipe->next;
    fclose(stream);
    pthread_join(pipe->child, NULL);
    pthread_join(pipe->server, NULL);
    int result = pipe->result;
    free(pipe->command);
    free(pipe);
    return result;
}

FILE *popen(const char *command, const char *mode)
{
    FILE *stream;
    FB_LOCK();
    stream = fb_hAmigaPipeOpen(command, mode);
    FB_UNLOCK();
    return stream;
}

int pclose(FILE *stream)
{
    int result;
    FB_LOCK();
    result = fb_hAmigaPipeClose(stream);
    FB_UNLOCK();
    return result;
}

void fb_hAmigaClosePipes(void)
{
    FB_LOCK();
    while (pipes != NULL) fb_hAmigaPipeClose(pipes->stream);
    FB_UNLOCK();
}

/* end of amiga/pipe.c */
