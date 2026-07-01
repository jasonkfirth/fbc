/*
    FreeBASIC runtime library
    -------------------------

    File: fb_nuttx_tcp.c

    Purpose:

        Provide OPEN TCP, OPEN TCP SERVER, TCP ACCEPT, and EOC for the
        small NuttX runtime.

    Responsibilities:

        - parse the simple option strings emitted by FreeBASIC programs
        - create loopback TCP client and server sockets
        - expose connected sockets through the ordinary BASIC file table
        - provide non-blocking EOF/EOC probes for socket polling loops

    This file intentionally does NOT contain:

        - DNS resolver policy beyond numeric IPv4 addresses
        - IPv6 support
        - TLS or protocol helpers
*/

#include "fb.h"

#include <arpa/inet.h>
#if defined(__has_include)
# if __has_include(<netutils/netlib.h>)
#  include <netutils/netlib.h>
#  define FB_NUTTX_HAVE_NETLIB 1
# endif
#endif
#if defined(__has_include)
# if __has_include(<netutils/netinit.h>)
#  include <netutils/netinit.h>
#  define FB_NUTTX_HAVE_NETINIT 1
# endif
#endif
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/* Option parsing                                                            */
/* ------------------------------------------------------------------------- */

typedef struct FB_NUTTX_TCP_OPTIONS {
    char host[64];
    int port;
    int backlog;
    int timeout_ms;
    int have_host;
} FB_NUTTX_TCP_OPTIONS;

static void fb_nuttx_tcp_options_init(FB_NUTTX_TCP_OPTIONS *options)
{
    memset(options, 0, sizeof(*options));

    options->port = -1;
    options->backlog = 1;
    options->timeout_ms = 0;
}

static void fb_nuttx_tcp_trim(char *text)
{
    char *src;
    char *dst;
    size_t len;

    src = text;
    while ((*src != '\0') && isspace((unsigned char)*src))
        ++src;

    if (src != text) {
        dst = text;
        while (*src != '\0')
            *dst++ = *src++;
        *dst = '\0';
    }

    len = strlen(text);
    while ((len > 0) && isspace((unsigned char)text[len - 1]))
        text[--len] = '\0';
}

static int fb_nuttx_tcp_set_option(FB_NUTTX_TCP_OPTIONS *options,
    const char *key, const char *value)
{
    if (strcmp(key, "host") == 0) {
        if (strlen(value) >= sizeof(options->host))
            return -1;

        strcpy(options->host, value);
        options->have_host = 1;
        return 0;
    }

    if (strcmp(key, "port") == 0) {
        options->port = atoi(value);
        return 0;
    }

    if (strcmp(key, "backlog") == 0) {
        options->backlog = atoi(value);
        if (options->backlog <= 0)
            options->backlog = 1;
        return 0;
    }

    if (strcmp(key, "timeout") == 0) {
        options->timeout_ms = atoi(value);
        if (options->timeout_ms < 0)
            options->timeout_ms = 0;
        return 0;
    }

    return 0;
}

static int fb_nuttx_tcp_loopback_ready = 0;

static void fb_nuttx_tcp_prepare_loopback(void)
{
#if defined(FB_NUTTX_HAVE_NETLIB)
    struct in_addr address;

    if (fb_nuttx_tcp_loopback_ready != 0)
        return;

#if defined(FB_NUTTX_HAVE_NETINIT)
    netinit_bringup();
#endif

    /* NuttX builds the loopback driver into the image, but it still needs
       an address and an explicit ifup before localhost TCP tests can use it. */
    address.s_addr = htonl(0x7f000001u);
    netlib_set_ipv4addr("lo", &address);
    netlib_set_dripv4addr("lo", &address);

    address.s_addr = htonl(0xff000000u);
    netlib_set_ipv4netmask("lo", &address);
    netlib_ifup("lo");
#endif

    fb_nuttx_tcp_loopback_ready = 1;
}
static int fb_nuttx_tcp_parse_options(const FBSTRING *text,
    FB_NUTTX_TCP_OPTIONS *options)
{
    char *copy;
    char *part;
    char *next;
    char *equals;
    size_t len;
    int result;

    if ((text == NULL) || (text->data == NULL) || (options == NULL))
        return -1;

    if (text->len < 0)
        len = strlen(text->data);
    else
        len = (size_t)text->len;

    copy = (char *)malloc(len + 1);
    if (copy == NULL)
        return -1;

    memcpy(copy, text->data, len);
    copy[len] = '\0';

    fb_nuttx_tcp_options_init(options);

    result = 0;
    part = copy;

    while ((part != NULL) && (*part != '\0')) {
        next = strchr(part, ',');
        if (next != NULL)
            *next++ = '\0';

        fb_nuttx_tcp_trim(part);

        if (*part != '\0') {
            equals = strchr(part, '=');
            if (equals == NULL) {
                result = -1;
                break;
            }

            *equals++ = '\0';
            fb_nuttx_tcp_trim(part);
            fb_nuttx_tcp_trim(equals);

            if (fb_nuttx_tcp_set_option(options, part, equals) != 0) {
                result = -1;
                break;
            }
        }

        part = next;
    }

    free(copy);

    if ((result != 0) || (options->port <= 0) || (options->port > 65535))
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */
/* File table integration                                                    */
/* ------------------------------------------------------------------------- */

static int fb_nuttx_tcp_peer_file[FB_NUTTX_MAX_FILES];
static int fb_nuttx_tcp_peer_closed[FB_NUTTX_MAX_FILES];

static void fb_nuttx_tcp_reset_peer_state(const int32 file_num)
{
    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return;

    fb_nuttx_tcp_peer_file[file_num] = 0;
    fb_nuttx_tcp_peer_closed[file_num] = 0;
}

static void fb_nuttx_tcp_pair_files(const int32 file_a, const int32 file_b)
{
    if ((file_a <= 0) || (file_a >= FB_NUTTX_MAX_FILES))
        return;

    if ((file_b <= 0) || (file_b >= FB_NUTTX_MAX_FILES))
        return;

    fb_nuttx_tcp_peer_file[file_a] = file_b;
    fb_nuttx_tcp_peer_file[file_b] = file_a;
    fb_nuttx_tcp_peer_closed[file_a] = 0;
    fb_nuttx_tcp_peer_closed[file_b] = 0;
}

static int fb_nuttx_tcp_peer_is_closed(const int32 file_num)
{
    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return 1;

    return fb_nuttx_tcp_peer_closed[file_num] != 0;
}

static void fb_nuttx_tcp_mark_file_closed(const int32 file_num)
{
    int peer_file;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return;

    peer_file = fb_nuttx_tcp_peer_file[file_num];

    if ((peer_file > 0) && (peer_file < FB_NUTTX_MAX_FILES)) {
        fb_nuttx_tcp_peer_closed[peer_file] = 1;
        fb_nuttx_tcp_peer_file[peer_file] = 0;
    }

    fb_nuttx_tcp_reset_peer_state(file_num);
}

static int fb_nuttx_tcp_store_socket(const int32 file_num, int sock,
    int kind)
{
    FILE *stream;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    stream = fdopen(sock, "r+");
    if (stream == NULL)
        return -1;

    setvbuf(stream, NULL, _IONBF, 0);

    fb_FileClose(file_num);
    fb_nuttx_tcp_reset_peer_state(file_num);

    fb_nuttx_files[file_num] = stream;
    fb_nuttx_file_kind[file_num] = kind;
    fb_nuttx_file_record_len[file_num] = 0;

    return 0;
}

static int fb_nuttx_tcp_socket_for_file(const int32 file_num)
{
    FILE *stream;

    stream = fb_nuttx_stream_for_file(file_num);
    if (stream == NULL)
        return -1;

    return fileno(stream);
}

/* ------------------------------------------------------------------------- */
/* Localhost stream pairs                                                    */
/* ------------------------------------------------------------------------- */

/*
    NuttX has a real TCP stack, but QEMU loopback is not always a useful
    dependency for the compiler test suite.  FreeBASIC programs commonly use
    OPEN TCP SERVER and OPEN TCP against 127.0.0.1 for in-process tests, so
    this runtime provides a narrow localhost path backed by socketpair().

    This is deliberately not a general network emulator.  It only handles
    localhost streams inside the same process.  Non-local addresses still go
    through the real TCP socket path below.
*/

#if defined(AF_LOCAL) || defined(AF_UNIX)
#define FB_NUTTX_HAVE_LOCALTCP 1

#if defined(AF_LOCAL)
#define FB_NUTTX_LOCALTCP_FAMILY AF_LOCAL
#else
#define FB_NUTTX_LOCALTCP_FAMILY AF_UNIX
#endif

#define FB_NUTTX_LOCALTCP_MAX_LISTENERS 8

typedef struct FB_NUTTX_LOCALTCP_LISTENER {
    int in_use;
    int server_file;
    int port;
    int pending_fd;
    int pending_client_file;
    int has_pending;
} FB_NUTTX_LOCALTCP_LISTENER;

static FB_NUTTX_LOCALTCP_LISTENER fb_nuttx_localtcp_listeners[
    FB_NUTTX_LOCALTCP_MAX_LISTENERS];

static pthread_mutex_t fb_nuttx_localtcp_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fb_nuttx_localtcp_cond;
static int fb_nuttx_localtcp_cond_ready = 0;

static int fb_nuttx_localtcp_lock(void)
{
    int result;

    result = pthread_mutex_lock(&fb_nuttx_localtcp_mutex);

    if (result != 0)
        return result;

    if (fb_nuttx_localtcp_cond_ready == 0) {
        result = pthread_cond_init(&fb_nuttx_localtcp_cond, NULL);

        if (result != 0) {
            pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
            return result;
        }

        fb_nuttx_localtcp_cond_ready = 1;
    }

    return 0;
}

static int fb_nuttx_tcp_is_localhost(const FB_NUTTX_TCP_OPTIONS *options,
    int allow_any)
{
    if (options->have_host == 0)
        return 1;

    if (strcmp(options->host, "127.0.0.1") == 0)
        return 1;

    if (strcmp(options->host, "localhost") == 0)
        return 1;

    if (allow_any && (strcmp(options->host, "0.0.0.0") == 0))
        return 1;

    return 0;
}

static void fb_nuttx_localtcp_clear(FB_NUTTX_LOCALTCP_LISTENER *listener)
{
    if ((listener->has_pending != 0) && (listener->pending_fd >= 0))
        close(listener->pending_fd);

    if ((listener->pending_client_file > 0) &&
        (listener->pending_client_file < FB_NUTTX_MAX_FILES))
        fb_nuttx_tcp_peer_closed[listener->pending_client_file] = 1;

    memset(listener, 0, sizeof(*listener));
    listener->pending_fd = -1;
}

static void fb_nuttx_localtcp_reclaim_locked(void)
{
    FB_NUTTX_LOCALTCP_LISTENER *listener;
    int i;

    for (i = 0; i < FB_NUTTX_LOCALTCP_MAX_LISTENERS; ++i) {
        listener = &fb_nuttx_localtcp_listeners[i];

        if (listener->in_use == 0)
            continue;

        if ((listener->server_file <= 0) ||
            (listener->server_file >= FB_NUTTX_MAX_FILES) ||
            (fb_nuttx_file_kind[listener->server_file] !=
                FB_NUTTX_FILE_KIND_TCP_SERVER)) {
            fb_nuttx_localtcp_clear(listener);
        }
    }
}

static void fb_nuttx_localtcp_forget_file(const int file_num)
{
    FB_NUTTX_LOCALTCP_LISTENER *listener;
    int i;

    if (fb_nuttx_localtcp_lock() != 0)
        return;

    for (i = 0; i < FB_NUTTX_LOCALTCP_MAX_LISTENERS; ++i) {
        listener = &fb_nuttx_localtcp_listeners[i];

        if ((listener->in_use != 0) && (listener->server_file == file_num))
            fb_nuttx_localtcp_clear(listener);
    }

    pthread_cond_broadcast(&fb_nuttx_localtcp_cond);
    pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
}

static FB_NUTTX_LOCALTCP_LISTENER *fb_nuttx_localtcp_find_port_locked(
    int port)
{
    int i;

    for (i = 0; i < FB_NUTTX_LOCALTCP_MAX_LISTENERS; ++i) {
        if ((fb_nuttx_localtcp_listeners[i].in_use != 0) &&
            (fb_nuttx_localtcp_listeners[i].port == port))
            return &fb_nuttx_localtcp_listeners[i];
    }

    return NULL;
}

static FB_NUTTX_LOCALTCP_LISTENER *fb_nuttx_localtcp_find_file_locked(
    int file_num)
{
    int i;

    for (i = 0; i < FB_NUTTX_LOCALTCP_MAX_LISTENERS; ++i) {
        if ((fb_nuttx_localtcp_listeners[i].in_use != 0) &&
            (fb_nuttx_localtcp_listeners[i].server_file == file_num))
            return &fb_nuttx_localtcp_listeners[i];
    }

    return NULL;
}

static int fb_nuttx_localtcp_make_deadline(struct timespec *deadline,
    int timeout_ms)
{
    if (clock_gettime(CLOCK_REALTIME, deadline) != 0)
        return -1;

    deadline->tv_sec += timeout_ms / 1000;
    deadline->tv_nsec += (long)(timeout_ms % 1000) * 1000000L;

    if (deadline->tv_nsec >= 1000000000L) {
        ++deadline->tv_sec;
        deadline->tv_nsec -= 1000000000L;
    }

    return 0;
}

static int fb_nuttx_localtcp_wait(pthread_cond_t *cond,
    pthread_mutex_t *mutex, int timeout_ms)
{
    struct timespec deadline;

    if (timeout_ms <= 0)
        return pthread_cond_wait(cond, mutex);

    if (fb_nuttx_localtcp_make_deadline(&deadline, timeout_ms) != 0)
        return -1;

    return pthread_cond_timedwait(cond, mutex, &deadline);
}

static int fb_nuttx_localtcp_open_server(const int32 file_num,
    const FB_NUTTX_TCP_OPTIONS *options)
{
    FB_NUTTX_LOCALTCP_LISTENER *listener;
    int marker[2];
    int i;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    marker[0] = -1;
    marker[1] = -1;

    fb_FileClose(file_num);
    fb_nuttx_localtcp_forget_file(file_num);

    if (socketpair(FB_NUTTX_LOCALTCP_FAMILY, SOCK_STREAM, 0, marker) != 0)
        return -1;

    close(marker[1]);
    marker[1] = -1;

    if (fb_nuttx_tcp_store_socket(file_num, marker[0],
                                  FB_NUTTX_FILE_KIND_TCP_SERVER) != 0) {
        close(marker[0]);
        return -1;
    }

    if (fb_nuttx_localtcp_lock() != 0) {
        fb_FileClose(file_num);
        return -1;
    }
    fb_nuttx_localtcp_reclaim_locked();

    if (fb_nuttx_localtcp_find_port_locked(options->port) != NULL) {
        pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
        fb_FileClose(file_num);
        return -1;
    }

    listener = NULL;
    for (i = 0; i < FB_NUTTX_LOCALTCP_MAX_LISTENERS; ++i) {
        if (fb_nuttx_localtcp_listeners[i].in_use == 0) {
            listener = &fb_nuttx_localtcp_listeners[i];
            break;
        }
    }

    if (listener == NULL) {
        pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
        fb_FileClose(file_num);
        return -1;
    }

    listener->in_use = 1;
    listener->server_file = file_num;
    listener->port = options->port;
    listener->pending_fd = -1;
    listener->pending_client_file = 0;
    listener->has_pending = 0;

    pthread_cond_broadcast(&fb_nuttx_localtcp_cond);
    pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);

    fb_nuttx_tcp_timeout_ms[file_num] = options->timeout_ms;
    return 0;
}

static int fb_nuttx_localtcp_queue_client(int server_fd, int client_file,
    const FB_NUTTX_TCP_OPTIONS *options)
{
    FB_NUTTX_LOCALTCP_LISTENER *listener;
    int wait_result;

    if (fb_nuttx_localtcp_lock() != 0)
        return -1;
    fb_nuttx_localtcp_reclaim_locked();

    listener = fb_nuttx_localtcp_find_port_locked(options->port);
    if (listener == NULL) {
        pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
        return -1;
    }

    while (listener->has_pending != 0) {
        wait_result = fb_nuttx_localtcp_wait(&fb_nuttx_localtcp_cond,
            &fb_nuttx_localtcp_mutex, options->timeout_ms);

        if (wait_result != 0) {
            pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
            return -1;
        }

        fb_nuttx_localtcp_reclaim_locked();
        listener = fb_nuttx_localtcp_find_port_locked(options->port);
        if (listener == NULL) {
            pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
            return -1;
        }
    }

    listener->pending_fd = server_fd;
    listener->pending_client_file = client_file;
    listener->has_pending = 1;

    pthread_cond_broadcast(&fb_nuttx_localtcp_cond);
    pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);

    return 0;
}

static int fb_nuttx_localtcp_open_client(const int32 file_num,
    const FB_NUTTX_TCP_OPTIONS *options)
{
    int pair[2];

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    pair[0] = -1;
    pair[1] = -1;

    if (socketpair(FB_NUTTX_LOCALTCP_FAMILY, SOCK_STREAM, 0, pair) != 0)
        return -1;

    if (fb_nuttx_tcp_store_socket(file_num, pair[0],
                                  FB_NUTTX_FILE_KIND_TCP) != 0) {
        close(pair[0]);
        close(pair[1]);
        return -1;
    }

    pair[0] = -1;

    if (fb_nuttx_localtcp_queue_client(pair[1], file_num, options) != 0) {
        fb_FileClose(file_num);
        close(pair[1]);
        return -1;
    }

    return 0;
}

static int fb_nuttx_localtcp_accept(const int32 file_num)
{
    FB_NUTTX_LOCALTCP_LISTENER *listener;
    int client_file;
    int client_fd;
    int peer_file;
    int timeout_ms;
    int wait_result;

    timeout_ms = fb_nuttx_tcp_timeout_ms[file_num];

    if (fb_nuttx_localtcp_lock() != 0)
        return 0;
    fb_nuttx_localtcp_reclaim_locked();

    listener = fb_nuttx_localtcp_find_file_locked(file_num);
    if (listener == NULL) {
        pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
        return 0;
    }

    while (listener->has_pending == 0) {
        wait_result = fb_nuttx_localtcp_wait(&fb_nuttx_localtcp_cond,
            &fb_nuttx_localtcp_mutex, timeout_ms);

        if (wait_result != 0) {
            pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
            return 0;
        }

        fb_nuttx_localtcp_reclaim_locked();
        listener = fb_nuttx_localtcp_find_file_locked(file_num);
        if (listener == NULL) {
            pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);
            return 0;
        }
    }

    client_fd = listener->pending_fd;
    peer_file = listener->pending_client_file;
    listener->pending_fd = -1;
    listener->pending_client_file = 0;
    listener->has_pending = 0;

    pthread_cond_broadcast(&fb_nuttx_localtcp_cond);
    pthread_mutex_unlock(&fb_nuttx_localtcp_mutex);

    client_file = fb_FileFree();
    if (client_file == 0) {
        close(client_fd);
        return 0;
    }

    if (fb_nuttx_tcp_store_socket(client_file, client_fd,
                                  FB_NUTTX_FILE_KIND_TCP) != 0) {
        close(client_fd);
        return 0;
    }

    fb_nuttx_tcp_pair_files(client_file, peer_file);

    return client_file;
}

#else

static int fb_nuttx_tcp_is_localhost(const FB_NUTTX_TCP_OPTIONS *options,
    int allow_any)
{
    (void)options;
    (void)allow_any;
    return 0;
}

static int fb_nuttx_localtcp_open_server(const int32 file_num,
    const FB_NUTTX_TCP_OPTIONS *options)
{
    (void)file_num;
    (void)options;
    return -1;
}

static int fb_nuttx_localtcp_open_client(const int32 file_num,
    const FB_NUTTX_TCP_OPTIONS *options)
{
    (void)file_num;
    (void)options;
    return -1;
}

static int fb_nuttx_localtcp_accept(const int32 file_num)
{
    (void)file_num;
    return 0;
}

#endif

/* ------------------------------------------------------------------------- */
/* Socket creation                                                           */
/* ------------------------------------------------------------------------- */

static int fb_nuttx_tcp_connect_with_timeout(int sock,
                                             const struct sockaddr_in *addr,
                                             int timeout_ms)
{
    fd_set write_set;
    struct timeval timeout;
    socklen_t error_len;
    int saved_flags;
    int error_value;
    int result;

    if (timeout_ms <= 0)
        timeout_ms = 5000;

    saved_flags = fcntl(sock, F_GETFL, 0);
    if (saved_flags >= 0)
        fcntl(sock, F_SETFL, saved_flags | O_NONBLOCK);

    result = connect(sock, (const struct sockaddr *)addr, sizeof(*addr));
    if (result == 0) {
        if (saved_flags >= 0)
            fcntl(sock, F_SETFL, saved_flags);
        return 0;
    }

    if ((errno != EINPROGRESS) && (errno != EWOULDBLOCK)) {
        if (saved_flags >= 0)
            fcntl(sock, F_SETFL, saved_flags);
        return -1;
    }

    FD_ZERO(&write_set);
    FD_SET(sock, &write_set);
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    result = select(sock + 1, NULL, &write_set, NULL, &timeout);
    if (result <= 0) {
        if (saved_flags >= 0)
            fcntl(sock, F_SETFL, saved_flags);
        errno = ETIMEDOUT;
        return -1;
    }

    error_value = 0;
    error_len = sizeof(error_value);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error_value, &error_len) != 0) {
        if (saved_flags >= 0)
            fcntl(sock, F_SETFL, saved_flags);
        return -1;
    }

    if (saved_flags >= 0)
        fcntl(sock, F_SETFL, saved_flags);

    if (error_value != 0) {
        errno = error_value;
        return -1;
    }

    return 0;
}
int32 fb_FileOpenTcpServer(const FBSTRING *text, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len, const char *encoding)
{
    fb_nuttx_tcp_prepare_loopback();
    FB_NUTTX_TCP_OPTIONS options;
    struct sockaddr_in addr;
    int sock;
    int yes;

    (void)mode;
    (void)access;
    (void)lock;
    (void)len;
    (void)encoding;

    if (fb_nuttx_tcp_parse_options(text, &options) != 0)
        return -1;

    if (fb_nuttx_tcp_is_localhost(&options, 1))
        return fb_nuttx_localtcp_open_server(file_num, &options);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)options.port);

    if (options.have_host) {
        if (inet_pton(AF_INET, options.host, &addr.sin_addr) != 1) {
            close(sock);
            return -1;
        }
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }

    if (listen(sock, options.backlog) != 0) {
        close(sock);
        return -1;
    }

    if (fb_nuttx_tcp_store_socket(file_num, sock,
                                  FB_NUTTX_FILE_KIND_TCP_SERVER) != 0) {
        close(sock);
        return -1;
    }

    fb_nuttx_tcp_timeout_ms[file_num] = options.timeout_ms;
    return 0;
}

int32 fb_FileOpenTcp(const FBSTRING *text, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len, const char *encoding)
{
    fb_nuttx_tcp_prepare_loopback();
    FB_NUTTX_TCP_OPTIONS options;
    struct sockaddr_in addr;
    int sock;

    (void)mode;
    (void)access;
    (void)lock;
    (void)len;
    (void)encoding;

    if (fb_nuttx_tcp_parse_options(text, &options) != 0)
        return -1;

    if (!options.have_host)
        strcpy(options.host, "127.0.0.1");

    if (fb_nuttx_tcp_is_localhost(&options, 0)) {
        if (fb_nuttx_localtcp_open_client(file_num, &options) == 0)
            return 0;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)options.port);

    if (inet_pton(AF_INET, options.host, &addr.sin_addr) != 1) {
        close(sock);
        return -1;
    }

    if (fb_nuttx_tcp_connect_with_timeout(sock, &addr, options.timeout_ms) != 0) {
        close(sock);
        return -1;
    }

    if (fb_nuttx_tcp_store_socket(file_num, sock,
                                  FB_NUTTX_FILE_KIND_TCP) != 0) {
        close(sock);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* TCP ACCEPT and polling                                                    */
/* ------------------------------------------------------------------------- */

int32 fb_TcpAccept(const int32 file_num)
{
    struct timeval timeout;
    fd_set read_set;
    int listener;
    int client;
    int client_file;
    int timeout_ms;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return 0;

    if (fb_nuttx_file_kind[file_num] != FB_NUTTX_FILE_KIND_TCP_SERVER)
        return 0;

    client_file = fb_nuttx_localtcp_accept(file_num);
    if (client_file != 0)
        return client_file;

    listener = fb_nuttx_tcp_socket_for_file(file_num);
    if (listener < 0)
        return 0;

    timeout_ms = fb_nuttx_tcp_timeout_ms[file_num];

    if (timeout_ms > 0) {
        FD_ZERO(&read_set);
        FD_SET(listener, &read_set);

        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        if (select(listener + 1, &read_set, NULL, NULL, &timeout) <= 0)
            return 0;
    }

    client = accept(listener, NULL, NULL);
    if (client < 0)
        return 0;

    client_file = fb_FileFree();
    if (client_file == 0) {
        close(client);
        return 0;
    }

    if (fb_nuttx_tcp_store_socket(client_file, client,
                                  FB_NUTTX_FILE_KIND_TCP) != 0) {
        close(client);
        return 0;
    }

    return client_file;
}

int32 fb_Eoc(const int32 file_num)
{
    char byte;
    int sock;
    int got;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    if (fb_nuttx_file_kind[file_num] == FB_NUTTX_FILE_KIND_TCP_SERVER)
        return 0;

    if (fb_nuttx_file_kind[file_num] != FB_NUTTX_FILE_KIND_TCP)
        return -1;

    sock = fb_nuttx_tcp_socket_for_file(file_num);
    if (sock < 0)
        return -1;

    got = recv(sock, &byte, 1, MSG_PEEK | MSG_DONTWAIT);

    if (got == 0)
        return -1;

    if (got > 0)
        return 0;

    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        if (fb_nuttx_tcp_peer_is_closed(file_num))
            return -1;

        return 0;
    }

    return -1;
}

static int32 fb_nuttx_tcp_file_eof(const int32 file_num)
{
    char byte;
    int sock;
    int got;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    if (fb_nuttx_file_kind[file_num] == FB_NUTTX_FILE_KIND_TCP_SERVER)
        return -1;

    if (fb_nuttx_file_kind[file_num] != FB_NUTTX_FILE_KIND_TCP)
        return -1;

    sock = fb_nuttx_tcp_socket_for_file(file_num);
    if (sock < 0)
        return -1;

    got = recv(sock, &byte, 1, MSG_PEEK | MSG_DONTWAIT);

    if (got > 0)
        return 0;

    return -1;
}

/* end of fb_nuttx_tcp.c */
