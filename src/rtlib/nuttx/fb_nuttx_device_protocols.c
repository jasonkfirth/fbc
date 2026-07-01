/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_device_protocols.c

    Purpose:

        Provide the hardware-free device protocol parsers needed by the
        generated-C NuttX test runtime.

    Responsibilities:

        - parse COM-style device names
        - parse LPT/PRN-style printer device names
        - parse TCP client/server option strings
        - expose the same parser symbols used by the normal rtlib

    This file intentionally does NOT contain:

        - serial-port I/O
        - printer I/O
        - socket creation or network transfer logic
        - platform-specific device drivers
*/

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <strings.h>

typedef struct FB_NUTTX_DEV_LPT_PROTOCOL {
    char *proto;
    int iPort;
    char *name;
    char *title;
    char *emu;
    char raw[];
} FB_NUTTX_DEV_LPT_PROTOCOL;

typedef struct FB_NUTTX_DEV_TCP_PROTOCOL {
    char *host;
    unsigned int port;
    unsigned int timeout;
    unsigned int backlog;
    int is_server;
    char raw[];
} FB_NUTTX_DEV_TCP_PROTOCOL;

int fb_DevComTestProtocolEx(void *handle, const char *filename,
    size_t filename_len, size_t *pPort)
{
    size_t i;
    size_t port;

    (void)handle;

    if (pPort != NULL)
        *pPort = 0;

    if ((filename == NULL) || (filename_len == 0))
        return 0;

    if ((filename_len >= 4) && (strncasecmp(filename, "SER:", 4) == 0)) {
        if (pPort != NULL)
            *pPort = 1;

        return 1;
    }

    if (filename_len < 4)
        return 0;

    if (strncasecmp(filename, "COM", 3) != 0)
        return memchr(filename, ':', filename_len) != NULL;

    port = 0;
    i = 3;

    while (i < filename_len) {
        size_t digit;
        char ch;

        ch = filename[i];

        if ((ch < '0') || (ch > '9'))
            break;

        digit = (size_t)(ch - '0');

        if (port > (((size_t)INT_MAX - digit) / 10))
            return 0;

        port = (port * 10) + digit;
        i++;
    }

    if ((i >= filename_len) || (filename[i] != ':'))
        return 0;

    if (pPort != NULL)
        *pPort = port;

    return 1;
}

int fb_DevComTestProtocol(void *handle, const char *filename,
    size_t filename_len)
{
    return fb_DevComTestProtocolEx(handle, filename, filename_len, NULL);
}

static int fb_nuttx_lpt_parse_port(const char *proto, int *port)
{
    const char *digits;
    int value;

    if (port == NULL)
        return 0;

    *port = 0;

    if (strncasecmp(proto, "LPT", 3) != 0)
        return 0;

    digits = proto + 3;

    if (*digits == '\0')
        return 1;

    value = 0;

    while (*digits != '\0') {
        int digit;

        if ((*digits < '0') || (*digits > '9'))
            return 0;

        digit = *digits - '0';

        if (value > ((INT_MAX - digit) / 10))
            return 0;

        value = (value * 10) + digit;
        digits++;
    }

    if (value <= 0)
        return 0;

    *port = value;

    return 1;
}

int fb_DevLptParseProtocol(FB_NUTTX_DEV_LPT_PROTOCOL **lpt_proto_out,
    const char *proto_raw, size_t proto_raw_len, int subst_prn)
{
    char *p;
    char *ptail;
    char *pc;
    char *pe;
    FB_NUTTX_DEV_LPT_PROTOCOL *lpt_proto;

    if ((proto_raw == NULL) || (lpt_proto_out == NULL))
        return 0;

    *lpt_proto_out = calloc(sizeof(FB_NUTTX_DEV_LPT_PROTOCOL) +
        proto_raw_len + 2, 1);

    lpt_proto = *lpt_proto_out;

    if (lpt_proto == NULL)
        return 0;

    strncpy(lpt_proto->raw, proto_raw, proto_raw_len);
    lpt_proto->raw[proto_raw_len] = '\0';

    p = lpt_proto->raw;
    ptail = p + strlen(lpt_proto->raw);

    lpt_proto->iPort = 0;
    lpt_proto->proto = ptail;
    lpt_proto->name = ptail;
    lpt_proto->title = ptail;
    lpt_proto->emu = ptail;

    if (strcasecmp(p, "PRN:") == 0) {
        if (subst_prn != 0)
            memcpy(p, "LPT1:", sizeof("LPT1:"));

        lpt_proto->proto = p;
        lpt_proto->iPort = 1;

        return 1;
    }

    if (strncasecmp(p, "LPT", 3) != 0)
        return 0;

    pc = strchr(p, ':');

    if (pc == NULL)
        return 0;

    lpt_proto->proto = p;
    p = pc + 1;
    *pc = '\0';

    if (fb_nuttx_lpt_parse_port(lpt_proto->proto, &lpt_proto->iPort) == 0)
        return 0;

    while (*p != '\0') {
        if (isspace((unsigned char)*p) || (*p == ',')) {
            p++;
        } else {
            char *pt;

            pe = strchr(p, '=');
            pc = strchr(p, ',');

            if ((pc != NULL) && (pe != NULL) && (pe > pc))
                pe = NULL;

            if (pe == NULL) {
                lpt_proto->name = p;
            } else {
                pt = pe;

                while ((pt > p) && isspace((unsigned char)*(pt - 1))) {
                    pt--;
                    *pt = '\0';
                }

                *pe = '\0';
                pe++;

                while (isspace((unsigned char)*pe)) {
                    *pe = '\0';
                    pe++;
                }

                if (strcasecmp(p, "EMU") == 0) {
                    lpt_proto->emu = pe;
                } else if (strcasecmp(p, "TITLE") == 0) {
                    lpt_proto->title = pe;
                }
            }

            pt = (pc != NULL) ? pc : ptail;

            while ((pt > p) && isspace((unsigned char)*(pt - 1))) {
                pt--;
                *pt = '\0';
            }

            if (pc != NULL) {
                p = pc + 1;
                *pc = '\0';
            } else {
                p = ptail;
            }
        }
    }

    return 1;
}

int fb_DevLptTestProtocol(void *handle, const char *filename,
    size_t filename_len)
{
    FB_NUTTX_DEV_LPT_PROTOCOL *lpt_proto;
    int result;

    (void)handle;

    lpt_proto = NULL;
    result = fb_DevLptParseProtocol(&lpt_proto, filename, filename_len, 0);

    if (lpt_proto != NULL)
        free(lpt_proto);

    return result;
}

static char *fb_nuttx_protocol_tcp_trim(char *p)
{
    char *end;

    while (isspace((unsigned char)*p))
        p++;

    end = p + strlen(p);

    while (end > p) {
        if (isspace((unsigned char)end[-1]) == 0)
            break;

        end--;
    }

    *end = '\0';

    return p;
}

static int fb_nuttx_tcp_parse_uint(const char *text, unsigned int *value)
{
    char *end;
    unsigned long parsed;

    if ((text == NULL) || (*text == '\0'))
        return 0;

    if (isdigit((unsigned char)*text) == 0)
        return 0;

    errno = 0;
    parsed = strtoul(text, &end, 10);

    if (*end != '\0')
        return 0;

    if (errno == ERANGE)
        return 0;

    if (parsed > UINT_MAX)
        return 0;

    *value = (unsigned int)parsed;

    return 1;
}

int fb_DevTcpParseProtocol(FB_NUTTX_DEV_TCP_PROTOCOL **tcp_proto_out,
    const char *proto_raw, size_t proto_raw_len, int is_server)
{
    char *p;
    FB_NUTTX_DEV_TCP_PROTOCOL *tcp_proto;

    if ((proto_raw == NULL) || (tcp_proto_out == NULL))
        return 0;

    *tcp_proto_out = calloc(sizeof(FB_NUTTX_DEV_TCP_PROTOCOL) +
        proto_raw_len + 2, 1);

    tcp_proto = *tcp_proto_out;

    if (tcp_proto == NULL)
        return 0;

    memcpy(tcp_proto->raw, proto_raw, proto_raw_len);
    tcp_proto->raw[proto_raw_len] = '\0';

    tcp_proto->host = tcp_proto->raw + proto_raw_len;
    tcp_proto->port = 0;
    tcp_proto->timeout = 0;
    tcp_proto->backlog = 16;
    tcp_proto->is_server = is_server;

    p = tcp_proto->raw;

    while (*p != '\0') {
        char *entry;
        char *comma;
        char *eq;
        char *key;
        char *value;
        unsigned int parsed;

        while ((*p == ',') || isspace((unsigned char)*p))
            p++;

        if (*p == '\0')
            break;

        entry = p;
        comma = strchr(entry, ',');

        if (comma != NULL) {
            *comma = '\0';
            p = comma + 1;
        } else {
            p = entry + strlen(entry);
        }

        eq = strchr(entry, '=');

        if (eq == NULL)
            return 0;

        *eq = '\0';
        key = fb_nuttx_protocol_tcp_trim(entry);
        value = fb_nuttx_protocol_tcp_trim(eq + 1);

        if (strcasecmp(key, "host") == 0) {
            tcp_proto->host = value;
        } else if (strcasecmp(key, "port") == 0) {
            if (fb_nuttx_tcp_parse_uint(value, &parsed) == 0)
                return 0;

            if (parsed > 65535u)
                return 0;

            tcp_proto->port = parsed;
        } else if (strcasecmp(key, "timeout") == 0) {
            if (fb_nuttx_tcp_parse_uint(value, &parsed) == 0)
                return 0;

            tcp_proto->timeout = parsed;
        } else if (strcasecmp(key, "backlog") == 0) {
            if (fb_nuttx_tcp_parse_uint(value, &parsed) == 0)
                return 0;

            tcp_proto->backlog = parsed;
        }
    }

    if (tcp_proto->port == 0)
        return 0;

    if ((is_server == 0) &&
        ((tcp_proto->host == NULL) || (*tcp_proto->host == '\0'))) {
        return 0;
    }

    return 1;
}

/* end of fb_nuttx_device_protocols.c */
