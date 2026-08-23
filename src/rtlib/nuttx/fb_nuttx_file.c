/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_file.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    Responsibilities:

        - implement BASIC file-number and C stdio operations
        - open and configure NuttX serial character devices for OPEN COM
        - expose fbcom.bi modem, queue, break, and purge controls
        - preserve and restore per-handle terminal configuration

    This file intentionally does NOT contain:

        - board-specific UART drivers or pin assignments
        - serial-device discovery
        - the normal hosted rtlib VFS and DEV_COM_INFO layers

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

/*
    The shared C stdio file-copy helper uses the normal rtlib file-open hook.
    NuttX does not need path conversion here, so the hook maps straight to
    fopen() while preserving the generic helper's interface.
*/
FILE *fb_hOpenFile(const char *path, const char *mode)
{
    if ((path == NULL) || (mode == NULL))
        return NULL;

    return fopen(path, mode);
}

int32 fb_FileClose(const int32 file_num)
{
    int file_kind;

    if (file_num == 0) {
        int32 i;

        fb_nuttx_input_file_num = 0;

        for (i = 1; i < FB_NUTTX_MAX_FILES; i++)
            fb_FileClose(i);

        return 0;
    }

    if ((file_num < 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    if (file_num == fb_nuttx_input_file_num) {
        fb_nuttx_input_file_num = 0;
        fb_nuttx_input_has_line = 0;
        fb_nuttx_input_pos = 0;
    }

    file_kind = fb_nuttx_file_kind[file_num];

#if FB_NUTTX_HAVE_SERIAL_TERMIOS
    if ((file_kind == FB_NUTTX_FILE_KIND_COM) &&
        (fb_nuttx_files[file_num] != NULL) &&
        (fb_nuttx_serial_old_termios_valid[file_num] != 0)) {
        (void)tcsetattr(fileno(fb_nuttx_files[file_num]), TCSAFLUSH,
                       &fb_nuttx_serial_old_termios[file_num]);
    }
#endif

#if FB_NUTTX_HAVE_TCP
    if (fb_nuttx_files[file_num] != NULL) {
        if ((file_kind == FB_NUTTX_FILE_KIND_TCP) ||
            (file_kind == FB_NUTTX_FILE_KIND_TCP_SERVER)) {
            int fd;

            fb_nuttx_tcp_mark_file_closed(file_num);

            fd = fileno(fb_nuttx_files[file_num]);

            if (fd >= 0)
                shutdown(fd, SHUT_RDWR);
        }

        fclose(fb_nuttx_files[file_num]);
        fb_nuttx_files[file_num] = NULL;
    }
#else
    (void)file_kind;

    if (fb_nuttx_files[file_num] != NULL) {
        fclose(fb_nuttx_files[file_num]);
        fb_nuttx_files[file_num] = NULL;
    }
#endif

    fb_nuttx_file_kind[file_num] = FB_NUTTX_FILE_KIND_NONE;
    fb_nuttx_tcp_timeout_ms[file_num] = 0;
    fb_nuttx_file_record_len[file_num] = 0;
    fb_nuttx_serial_output_lines[file_num] = 0;
#if FB_NUTTX_HAVE_SERIAL_TERMIOS
    fb_nuttx_serial_old_termios_valid[file_num] = 0;
#endif

    return 0;
}

int32 fb_FileCloseAll(void)
{
    return fb_FileClose(0);
}

int32 fb_FileFlush(const int32 file_num)
{
    FILE *stream;
    int32 i;
    int result;

    if (file_num == 0) {
        result = 0;

        for (i = 1; i < FB_NUTTX_MAX_FILES; i++) {
            if (fb_nuttx_files[i] != NULL) {
                if (fflush(fb_nuttx_files[i]) != 0)
                    result = -1;
            }
        }

        return result;
    }

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    if (fflush(stream) != 0)
        return -1;

    return 0;
}

int32 fb_FileSetEof(const int32 file_num)
{
    FILE *stream;
    long position;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    if (fflush(stream) != 0)
        return -1;

    position = ftell(stream);

    if (position < 0)
        return -1;

    if (ftruncate(fileno(stream), (off_t)position) != 0)
        return -1;

    return 0;
}

void fb_FileReset(void)
{
    (void)fb_FileClose(0);
}

int32 fb_FileLock(const int32 file_num, const uint32 start, const uint32 end)
{
    (void)start;
    (void)end;

    if (fb_nuttx_stream_for_file(file_num) == NULL)
        return -1;

    return 0;
}

int32 fb_FileUnlock(const int32 file_num, const uint32 start, const uint32 end)
{
    (void)start;
    (void)end;

    if (fb_nuttx_stream_for_file(file_num) == NULL)
        return -1;

    return 0;
}

int32 fb_FileInput(const int32 file_num)
{
    if (fb_nuttx_stream_for_file(file_num) == NULL)
        return -1;

    fb_nuttx_input_file_num = file_num;
    fb_nuttx_input_has_line = 0;
    fb_nuttx_input_pos = 0;

    return 0;
}

int32 fb_FileFree(void)
{
    int32 i;

    for (i = 1; i < FB_NUTTX_MAX_FILES; i++) {
        if (fb_nuttx_files[i] == NULL)
            return i;
    }

    return 0;
}

int32 fb_FileOpen(const FBSTRING *filename, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len)
{
    char *path;
    const char *c_mode;

    (void)access;
    (void)lock;
    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    path = fb_nuttx_string_to_cstr(filename);

    if (path == NULL)
        return -1;

    switch (mode) {
    case 0:
    case 1:
        c_mode = NULL;
        break;

    case 2:
        c_mode = "r";
        break;

    case 3:
        c_mode = "w";
        break;

    case 4:
        c_mode = "a";
        break;

    default:
        free(path);
        return -1;
    }

    fb_FileClose(file_num);

    fb_nuttx_file_kind[file_num] = FB_NUTTX_FILE_KIND_FILE;
    fb_nuttx_file_record_len[file_num] = 0;

    if ((mode == 0) || (mode == 1)) {
        /*
            FreeBASIC's binary mode is read/write byte access.  C does not
            have one fopen mode that both preserves an existing file and
            creates a missing one, so try the preserving path first. Random
            mode uses the same stream mode, with per-handle record seeking.
        */
        fb_nuttx_files[file_num] = fopen(path, "r+b");

        if (fb_nuttx_files[file_num] == NULL)
            fb_nuttx_files[file_num] = fopen(path, "w+b");
    } else {
        fb_nuttx_files[file_num] = fopen(path, c_mode);
    }

    free(path);

    if (fb_nuttx_files[file_num] == NULL)
        return -1;

    if ((mode == 1) && (len > 0))
        fb_nuttx_file_record_len[file_num] = len;

    return 0;
}

static int fb_nuttx_string_equals_ascii_ci(const FBSTRING *text,
    const char *expected)
{
    int32 i;
    char a;
    char b;

    if ((text == NULL) || (text->data == NULL) || (expected == NULL))
        return 0;

    for (i = 0; i < text->len; i++) {
        a = text->data[i];
        b = expected[i];

        if (b == '\0')
            return 0;

        if ((a >= 'A') && (a <= 'Z'))
            a = (char)(a - 'A' + 'a');

        if ((b >= 'A') && (b <= 'Z'))
            b = (char)(b - 'A' + 'a');

        if (a != b)
            return 0;
    }

    return expected[i] == '\0';
}

static int32 fb_nuttx_file_open_console(const uint32 mode, const int32 file_num)
{
    int fd;
    FILE *stream;

    if ((file_num < 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return -1;

    /*
        QuickBASIC accepted CONS: and SCRN: as device names.

        The normal rtlib routes those through its VFS device layer.  The
        compact NuttX runtime does not have that layer, so duplicate the
        process console descriptors instead.  Duplicating is important:
        CLOSE #n must close only the BASIC file handle, not stdin/stdout
        themselves.
    */
    if (mode == 2)
        fd = dup(0);
    else
        fd = dup(1);

    if (fd < 0)
        return -1;

    if (mode == 2)
        stream = fdopen(fd, "r");
    else
        stream = fdopen(fd, "w");

    if (stream == NULL) {
        close(fd);
        return -1;
    }

    fb_FileClose(file_num);

    fb_nuttx_files[file_num] = stream;
    fb_nuttx_file_kind[file_num] = FB_NUTTX_FILE_KIND_FILE;
    fb_nuttx_file_record_len[file_num] = 0;

    return 0;
}

int32 fb_FileOpenQB(FBSTRING *filename, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len)
{
    if ((filename == NULL) || (filename->data == NULL) || (filename->len <= 0))
        return -1;

    if (fb_nuttx_string_equals_ascii_ci(filename, "cons:") ||
        fb_nuttx_string_equals_ascii_ci(filename, "scrn:")) {
        return fb_nuttx_file_open_console(mode, file_num);
    }

    return fb_FileOpen(filename, mode, access, lock, file_num, len);
}

int32 fb_FileOpenEncod(const FBSTRING *filename, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len, const char *encoding)
{
    /*
        Encoding-aware text conversion is not part of the seed NuttX runtime
        yet.  The open mode and file handle semantics are still identical for
        the LOF and binary tests, so keep the parameter accepted and ignored.
    */
    (void)encoding;

    return fb_FileOpen(filename, mode, access, lock, file_num, len);
}

int32 fb_FilePut(const int32 file_num, const int32 position,
    const void *src, const uint32 bytes)
{
    FILE *stream;
    size_t written;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (src == NULL))
        return -1;

    if (fb_nuttx_file_seek_position(stream, file_num, position) != 0)
        return -1;

    if (bytes == 0)
        return 0;

    written = fwrite(src, 1, (size_t)bytes, stream);
    fflush(stream);

    if (written != (size_t)bytes)
        return -1;

    return 0;
}

int32 fb_FileGet(const int32 file_num, const int32 position,
    void *dst, const uint32 bytes)
{
    FILE *stream;
    size_t got;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (dst == NULL))
        return -1;

    if (fb_nuttx_file_seek_position(stream, file_num, position) != 0)
        return -1;

    if (bytes == 0)
        return 0;

    got = fread(dst, 1, (size_t)bytes, stream);

    if (got != (size_t)bytes)
        return -1;

    return 0;
}

static int32 fb_nuttx_file_seek_position64(FILE *stream, const int32 file_num,
    const int64_t position)
{
    int64_t offset;

    if (position <= 0)
        return 0;

    offset = position - 1;

    if ((file_num > 0) && (file_num < FB_NUTTX_MAX_FILES) &&
        (fb_nuttx_file_record_len[file_num] > 0)) {
        offset *= (int64_t)fb_nuttx_file_record_len[file_num];
    }

    if ((offset > LONG_MAX) || (offset < LONG_MIN))
        return -1;

    if (fseek(stream, (long)offset, SEEK_SET) != 0)
        return -1;

    return 0;
}

static int32 fb_nuttx_file_get_data(const int32 file_num,
    const int64_t position, void *dst, const uint32 bytes,
    uint32 *bytes_read)
{
    FILE *stream;
    size_t got;

    if (bytes_read != NULL)
        *bytes_read = 0;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (dst == NULL))
        return -1;

    if (fb_nuttx_file_seek_position64(stream, file_num, position) != 0)
        return -1;

    if (bytes == 0)
        return 0;

    got = fread(dst, 1, (size_t)bytes, stream);

    if (bytes_read != NULL)
        *bytes_read = (uint32)got;

    if (got != (size_t)bytes)
        return -1;

    return 0;
}

static int32 fb_nuttx_file_get_str_data(const int32 file_num,
    const int64_t position, void *dst, const int32 dst_len,
    uint32 *bytes_read)
{
    FILE *stream;
    FBSTRING *dst_string;
    char *buffer;
    int32 len;
    size_t got;

    if (bytes_read != NULL)
        *bytes_read = 0;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (dst == NULL))
        return -1;

    if (fb_nuttx_file_seek_position64(stream, file_num, position) != 0)
        return -1;

    if (fb_nuttx_is_fixed_length(dst_len)) {
        len = fb_nuttx_decode_fixed_length(dst_len);
    } else {
        dst_string = (FBSTRING *)dst;
        len = dst_string->len;
    }

    if (len <= 0) {
        fb_StrAssign(dst, dst_len, "", 1, 0);
        return 0;
    }

    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return -1;

    got = fread(buffer, 1, (size_t)len, stream);
    buffer[got] = '\0';

    if (bytes_read != NULL)
        *bytes_read = (uint32)got;

    fb_StrAssign(dst, dst_len, buffer, (int32)got + 1, 0);
    free(buffer);

    return 0;
}

static int32 fb_nuttx_wstr_current_len(const uint32_t *text)
{
    int32 len;

    if (text == NULL)
        return 0;

    len = 0;

    while ((text[len] != 0) && (len < INT32_MAX))
        len++;

    return len;
}

static int32 fb_nuttx_file_get_wstr_data(const int32 file_num,
    const int64_t position, uint32_t *dst, int32 dst_chars,
    uint32 *bytes_read)
{
    FILE *stream;
    size_t bytes_to_read;
    size_t got;
    size_t chars_read;

    if (bytes_read != NULL)
        *bytes_read = 0;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (dst == NULL))
        return -1;

    if (dst_chars == 0)
        dst_chars = fb_nuttx_wstr_current_len(dst) + 1;

    if (dst_chars < 2)
        return -1;

    if (fb_nuttx_file_seek_position64(stream, file_num, position) != 0)
        return -1;

    bytes_to_read = ((size_t)dst_chars - 1) * sizeof(uint32_t);
    memset(dst, 0, (size_t)dst_chars * sizeof(uint32_t));

    got = fread(dst, 1, bytes_to_read, stream);

    if (bytes_read != NULL)
        *bytes_read = (uint32)got;

    chars_read = got / sizeof(uint32_t);

    if ((got % sizeof(uint32_t)) != 0)
        chars_read++;

    if (chars_read >= (size_t)dst_chars)
        chars_read = (size_t)dst_chars - 1;

    dst[chars_read] = 0;

    return 0;
}

int32 fb_FileGetLarge(const int32 file_num, const int64_t position,
    void *dst, const uint32 bytes)
{
    return fb_nuttx_file_get_data(file_num, position, dst, bytes, NULL);
}

int32 fb_FileGetIOB(const int32 file_num, const int32 position,
    void *dst, const uint32 bytes, uint32 *bytes_read)
{
    return fb_nuttx_file_get_data(file_num, position, dst, bytes, bytes_read);
}

int32 fb_FileGetLargeIOB(const int32 file_num, const int64_t position,
    void *dst, const uint32 bytes, uint32 *bytes_read)
{
    return fb_nuttx_file_get_data(file_num, position, dst, bytes, bytes_read);
}

int32 fb_FileGetStrLarge(const int32 file_num, const int64_t position,
    void *dst, const int32 dst_len)
{
    return fb_nuttx_file_get_str_data(file_num, position, dst, dst_len, NULL);
}

int32 fb_FileGetStrIOB(const int32 file_num, const int32 position,
    void *dst, const int32 dst_len, uint32 *bytes_read)
{
    return fb_nuttx_file_get_str_data(file_num, position, dst, dst_len,
        bytes_read);
}

int32 fb_FileGetStrLargeIOB(const int32 file_num, const int64_t position,
    void *dst, const int32 dst_len, uint32 *bytes_read)
{
    return fb_nuttx_file_get_str_data(file_num, position, dst, dst_len,
        bytes_read);
}

int32 fb_FileGetWstr(const int32 file_num, const int32 position,
    uint32_t *dst, const int32 dst_chars)
{
    return fb_nuttx_file_get_wstr_data(file_num, position, dst, dst_chars,
        NULL);
}

int32 fb_FileGetWstrLarge(const int32 file_num, const int64_t position,
    uint32_t *dst, const int32 dst_chars)
{
    return fb_nuttx_file_get_wstr_data(file_num, position, dst, dst_chars,
        NULL);
}

int32 fb_FileGetWstrIOB(const int32 file_num, const int32 position,
    uint32_t *dst, const int32 dst_chars, uint32 *bytes_read)
{
    return fb_nuttx_file_get_wstr_data(file_num, position, dst, dst_chars,
        bytes_read);
}

int32 fb_FileGetWstrLargeIOB(const int32 file_num, const int64_t position,
    uint32_t *dst, const int32 dst_chars, uint32 *bytes_read)
{
    return fb_nuttx_file_get_wstr_data(file_num, position, dst, dst_chars,
        bytes_read);
}

int32 fb_FileGetArray(const int32 file_num, const int32 position,
    FB_NUTTX_ARRAY *dst)
{
    if (dst == NULL)
        return -1;

    return fb_nuttx_file_get_data(file_num, position, dst->ptr,
        (uint32)dst->size, NULL);
}

int32 fb_FileGetArrayLarge(const int32 file_num, const int64_t position,
    FB_NUTTX_ARRAY *dst)
{
    if (dst == NULL)
        return -1;

    return fb_nuttx_file_get_data(file_num, position, dst->ptr,
        (uint32)dst->size, NULL);
}

int32 fb_FileGetArrayIOB(const int32 file_num, const int32 position,
    FB_NUTTX_ARRAY *dst, uint32 *bytes_read)
{
    if (dst == NULL)
        return -1;

    return fb_nuttx_file_get_data(file_num, position, dst->ptr,
        (uint32)dst->size, bytes_read);
}

int32 fb_FileGetArrayLargeIOB(const int32 file_num, const int64_t position,
    FB_NUTTX_ARRAY *dst, uint32 *bytes_read)
{
    if (dst == NULL)
        return -1;

    return fb_nuttx_file_get_data(file_num, position, dst->ptr,
        (uint32)dst->size, bytes_read);
}

int32 fb_FilePutStr(const int32 file_num, const int32 position,
    const void *src, const int32 src_len)
{
    FILE *stream;
    const char *data;
    int32 len;
    size_t written;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (src == NULL))
        return -1;

    if (fb_nuttx_file_seek_position(stream, file_num, position) != 0)
        return -1;

    len = fb_nuttx_source_length(src, src_len);

    if (len <= 0)
        return 0;

    data = fb_nuttx_source_data(src, src_len);

    if (data == NULL)
        return -1;

    written = fwrite(data, 1, (size_t)len, stream);
    fflush(stream);

    if (written != (size_t)len)
        return -1;

    return 0;
}

int32 fb_FileGetStr(const int32 file_num, const int32 position,
    void *dst, const int32 dst_len)
{
    FILE *stream;
    FBSTRING *dst_string;
    char *buffer;
    int32 len;
    size_t got;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (dst == NULL))
        return -1;

    if (fb_nuttx_file_seek_position(stream, file_num, position) != 0)
        return -1;

    if (fb_nuttx_is_fixed_length(dst_len)) {
        len = fb_nuttx_decode_fixed_length(dst_len);
    } else {
        dst_string = (FBSTRING *)dst;
        len = dst_string->len;
    }

    if (len <= 0) {
        fb_StrAssign(dst, dst_len, "", 1, 0);
        return 0;
    }

    buffer = (char *)malloc((size_t)len + 1);

    if (buffer == NULL)
        return -1;

    got = fread(buffer, 1, (size_t)len, stream);
    buffer[got] = '\0';

    fb_StrAssign(dst, dst_len, buffer, (int32)got + 1, 0);
    free(buffer);

    return 0;
}

int32 fb_FileLineInput(const int32 file_num, void *dst_void,
    const int32 dst_len, const int32 add_question)
{
    FILE *stream;
    char buffer[FB_NUTTX_LINE_BUFFER_SIZE];
    size_t len;

    (void)add_question;

    stream = fb_nuttx_stream_for_file(file_num);

    if ((stream == NULL) || (dst_void == NULL))
        return -1;

    if (fgets(buffer, sizeof(buffer), stream) == NULL) {
        fb_StrAssign(dst_void, dst_len, "", 1, 0);
        return -1;
    }

    len = strlen(buffer);

    while ((len > 0) && ((buffer[len - 1] == '\n') || (buffer[len - 1] == '\r'))) {
        len--;
        buffer[len] = '\0';
    }

    fb_StrAssign(dst_void, dst_len, buffer, (int32)len + 1, 0);

    return 0;
}

FBSTRING *fb_FileStrInput(const int32 chars, const int32 file_num)
{
    FILE *stream;
    char *buffer;
    size_t got;

    if (chars <= 0)
        return fb_nuttx_temp_string("", 0);

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return fb_nuttx_temp_string("", 0);

    buffer = (char *)malloc((size_t)chars + 1);

    if (buffer == NULL)
        return fb_nuttx_temp_string("", 0);

    got = fread(buffer, 1, (size_t)chars, stream);
    buffer[got] = '\0';

    return fb_nuttx_temp_string(buffer, (int32)got);
}

int64_t fb_FileSize(const int32 file_num)
{
    FILE *stream;
    long old_pos;
    long end_pos;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return 0;

    old_pos = ftell(stream);

    if (old_pos < 0)
        return 0;

    if (fseek(stream, 0, SEEK_END) != 0)
        return 0;

    end_pos = ftell(stream);

    if (fseek(stream, old_pos, SEEK_SET) != 0)
        return 0;

    if (end_pos < 0)
        return 0;

    return (int64_t)end_pos;
}

int64_t fb_FileLen(const char *filename)
{
    struct stat info;

    if (filename == NULL)
        return 0;

    if (stat(filename, &info) != 0)
        return 0;

    return (int64_t)info.st_size;
}

int32 fb_FileEof(const int32 file_num)
{
    FILE *stream;
    long pos;
    int64_t size;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    if ((file_num > 0) && (file_num < FB_NUTTX_MAX_FILES) &&
        (fb_nuttx_file_kind[file_num] == FB_NUTTX_FILE_KIND_COM)) {
#ifdef TIOCINQ
        int queued = 0;

        if (ioctl(fileno(stream), TIOCINQ,
                  (unsigned long)(uintptr_t)&queued) == 0)
            return queued == 0 ? -1 : 0;
#endif

        return -1;
    }

#if FB_NUTTX_HAVE_TCP
    if ((file_num > 0) && (file_num < FB_NUTTX_MAX_FILES) &&
        ((fb_nuttx_file_kind[file_num] == FB_NUTTX_FILE_KIND_TCP) ||
         (fb_nuttx_file_kind[file_num] == FB_NUTTX_FILE_KIND_TCP_SERVER))) {
        return fb_nuttx_tcp_file_eof(file_num);
    }
#endif

    pos = ftell(stream);

    if (pos < 0)
        return -1;

    size = fb_FileSize(file_num);

    if ((int64_t)pos >= size)
        return -1;

    return 0;
}

int32 fb_FileSeek(const int32 file_num, const int32 position)
{
    FILE *stream;
    long offset;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return -1;

    offset = 0;

    if (position > 1)
        offset = (long)position - 1;

    if (fseek(stream, offset, SEEK_SET) != 0)
        return -1;

    return 0;
}

int64_t fb_FileTell(const int32 file_num)
{
    FILE *stream;
    long pos;

    stream = fb_nuttx_stream_for_file(file_num);

    if (stream == NULL)
        return 0;

    pos = ftell(stream);

    if (pos < 0)
        return 0;

    return (int64_t)pos + 1;
}

int64_t fb_FileLocation(const int32 file_num)
{
    return fb_FileTell(file_num);
}

int32 fb_FileKill(const FBSTRING *filename)
{
    char *path;
    int result;

    path = fb_nuttx_string_to_cstr(filename);

    if (path == NULL)
        return -1;

    result = remove(path);
    free(path);

    if (result != 0)
        return -1;

    return 0;
}

#if !defined(FB_NUTTX_USE_GENERIC_DIR) || (FB_NUTTX_USE_GENERIC_DIR == 0)
int32 fb_MkDir(const FBSTRING *dirname)
{
    char *path;
    int result;

    path = fb_nuttx_string_to_cstr(dirname);

    if (path == NULL)
        return -1;

    result = mkdir(path, 0777);
    free(path);

    if (result != 0)
        return -1;

    return 0;
}

int32 fb_RmDir(const FBSTRING *dirname)
{
    char *path;
    int result;

    path = fb_nuttx_string_to_cstr(dirname);

    if (path == NULL)
        return -1;

    result = rmdir(path);
    free(path);

    if (result != 0)
        return -1;

    return 0;
}

int32 fb_ChDir(const FBSTRING *dirname)
{
    char *path;
    int result;

    path = fb_nuttx_string_to_cstr(dirname);

    if (path == NULL)
        return -1;

    result = chdir(path);
    free(path);

    if (result != 0)
        return -1;

    return 0;
}

ssize_t fb_hGetCurrentDir(char *dst, const ssize_t maxlen)
{
    if ((dst == NULL) || (maxlen <= 0))
        return 0;

    if (getcwd(dst, (size_t)maxlen) == NULL)
        return 0;

    return (ssize_t)strlen(dst);
}

FBSTRING *fb_CurDir(void)
{
    char path[256];
    char *stored_path;
    size_t path_len;

    if (getcwd(path, sizeof(path)) == NULL)
        return fb_StrAllocTempDescZEx("", 0);

    path_len = strlen(path);
    stored_path = (char *)malloc(path_len + 1);

    if (stored_path == NULL)
        return fb_StrAllocTempDescZEx("", 0);

    memcpy(stored_path, path, path_len + 1);

    /*
        The temporary string descriptor returned here must not point at the
        local stack buffer. FreeBASIC code commonly stores CURDIR() in a STRING
        and then calls other runtime functions before using that value again.
    */
    return fb_StrAllocTempDescZEx(stored_path, (int32)path_len);
}
#else

ssize_t fb_hGetCurrentDir(char *dst, const ssize_t maxlen)
{
    if ((dst == NULL) || (maxlen <= 0))
        return 0;

    if (getcwd(dst, (size_t)maxlen) == NULL)
        return 0;

    return (ssize_t)strlen(dst);
}
#endif

#if !defined(FB_NUTTX_USE_GENERIC_FILE_COPY) || \
    (FB_NUTTX_USE_GENERIC_FILE_COPY == 0)
int32 fb_FileCopy(const char *src_path, const char *dst_path)
{
    FILE *src;
    FILE *dst;
    unsigned char buffer[1024];
    size_t bytes_read;
    int32 result;

    if ((src_path == NULL) || (dst_path == NULL))
        return -1;

    src = fopen(src_path, "rb");

    if (src == NULL) {
        fb_nuttx_error_num = errno;
        return -1;
    }

    dst = fopen(dst_path, "wb");

    if (dst == NULL) {
        fb_nuttx_error_num = errno;
        fclose(src);
        return -1;
    }

    result = 0;

    for (;;) {
        bytes_read = fread(buffer, 1, sizeof(buffer), src);

        if (bytes_read > 0) {
            if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
                fb_nuttx_error_num = errno;
                result = -1;
                break;
            }
        }

        if (bytes_read < sizeof(buffer)) {
            if (ferror(src) != 0) {
                fb_nuttx_error_num = errno;
                result = -1;
            }

            break;
        }
    }

    if (fclose(dst) != 0)
        result = -1;

    fclose(src);

    return result;
}
#else
FBCALL int fb_CrtFileCopy(const char *source, const char *destination);

int32 fb_FileCopy(const char *src_path, const char *dst_path)
{
    return fb_CrtFileCopy(src_path, dst_path);
}
#endif

/* ------------------------------------------------------------------------- */
/* NuttX serial devices                                                      */
/* ------------------------------------------------------------------------- */

#define FB_NUTTX_SERIAL_PATH_MAX 256

static char *fb_nuttx_serial_trim(char *text)
{
    char *end;

    while ((*text == ' ') || (*text == '\t'))
        text++;

    end = text + strlen(text);

    while ((end > text) && ((end[-1] == ' ') || (end[-1] == '\t')))
        end--;

    *end = '\0';
    return text;
}

static int fb_nuttx_serial_parse_uint(const char *text, unsigned int *value)
{
    unsigned long parsed;
    char *end;

    if ((text == NULL) || (*text == '\0') || (value == NULL))
        return -1;

    errno = 0;
    parsed = strtoul(text, &end, 10);

    if ((errno != 0) || (*end != '\0') || (parsed > UINT_MAX))
        return -1;

    *value = (unsigned int)parsed;
    return 0;
}

static int fb_nuttx_serial_parse_extended(char *option,
    FB_SERIAL_OPTIONS *serial_options)
{
    unsigned int value;

    if (strcasecmp(option, "RS") == 0)
        serial_options->SuppressRTS = 1;
    else if ((strcasecmp(option, "LF") == 0) ||
             (strcasecmp(option, "ASC") == 0))
        serial_options->AddLF = 1;
    else if (strcasecmp(option, "BIN") == 0)
        serial_options->AddLF = 0;
    else if (strcasecmp(option, "PE") == 0)
        serial_options->CheckParity = 1;
    else if (strcasecmp(option, "DT") == 0)
        serial_options->KeepDTREnabled = 1;
    else if (strcasecmp(option, "FE") == 0)
        serial_options->DiscardOnError = 1;
    else if (strcasecmp(option, "ME") == 0)
        serial_options->IgnoreAllErrors = 1;
    else if ((strncasecmp(option, "CS", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->DurationCTS = value;
    else if ((strncasecmp(option, "DS", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->DurationDSR = value;
    else if ((strncasecmp(option, "CD", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->DurationCD = value;
    else if ((strncasecmp(option, "OP", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->OpenTimeout = value;
    else if ((strncasecmp(option, "TB", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->TransmitBuffer = value;
    else if ((strncasecmp(option, "RB", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->ReceiveBuffer = value;
    else if ((strncasecmp(option, "IR", 2) == 0) &&
             (fb_nuttx_serial_parse_uint(option + 2, &value) == 0))
        serial_options->IRQNumber = value;
    else
        return -1;

    return 0;
}

static int fb_nuttx_serial_parse_options(char *text,
    FB_SERIAL_OPTIONS *serial_options)
{
    unsigned int option_index;
    int stop_bits_set;
    char *cursor;

    if ((text == NULL) || (serial_options == NULL))
        return -1;

    memset(serial_options, 0, sizeof(*serial_options));
    serial_options->uiSpeed = 300;
    serial_options->Parity = FB_SERIAL_PARITY_EVEN;
    serial_options->uiDataBits = 7;
    serial_options->DurationCTS = 1000;
    serial_options->DurationDSR = 1000;
    stop_bits_set = 0;
    option_index = 0;
    cursor = text;

    for (;;) {
        char *next = strchr(cursor, ',');
        char *option;

        if (next != NULL)
            *next = '\0';

        option = fb_nuttx_serial_trim(cursor);

        if (*option != '\0') {
            switch (option_index) {
            case 0:
                if (fb_nuttx_serial_parse_uint(option,
                        &serial_options->uiSpeed) != 0)
                    return -1;
                break;

            case 1:
                if (strcasecmp(option, "N") == 0)
                    serial_options->Parity = FB_SERIAL_PARITY_NONE;
                else if (strcasecmp(option, "E") == 0)
                    serial_options->Parity = FB_SERIAL_PARITY_EVEN;
                else if (strcasecmp(option, "PE") == 0) {
                    serial_options->Parity = FB_SERIAL_PARITY_EVEN;
                    serial_options->CheckParity = 1;
                } else if (strcasecmp(option, "O") == 0)
                    serial_options->Parity = FB_SERIAL_PARITY_ODD;
                else if (strcasecmp(option, "S") == 0)
                    serial_options->Parity = FB_SERIAL_PARITY_SPACE;
                else if (strcasecmp(option, "M") == 0)
                    serial_options->Parity = FB_SERIAL_PARITY_MARK;
                else
                    return -1;
                break;

            case 2:
                if (fb_nuttx_serial_parse_uint(option,
                        &serial_options->uiDataBits) != 0)
                    return -1;
                break;

            case 3:
                if (strcmp(option, "1") == 0)
                    serial_options->StopBits = FB_SERIAL_STOP_BITS_1;
                else if (strcmp(option, "1.5") == 0)
                    serial_options->StopBits = FB_SERIAL_STOP_BITS_1_5;
                else if (strcmp(option, "2") == 0)
                    serial_options->StopBits = FB_SERIAL_STOP_BITS_2;
                else
                    return -1;

                stop_bits_set = 1;
                break;

            default:
                if (fb_nuttx_serial_parse_extended(option,
                        serial_options) != 0)
                    return -1;
                break;
            }
        }

        option_index++;

        if (next == NULL)
            break;

        cursor = next + 1;
    }

    if (stop_bits_set == 0) {
        if (serial_options->uiSpeed <= 110)
            serial_options->StopBits = (serial_options->uiDataBits == 5) ?
                FB_SERIAL_STOP_BITS_1_5 : FB_SERIAL_STOP_BITS_2;
        else
            serial_options->StopBits = FB_SERIAL_STOP_BITS_1;
    }

    if ((serial_options->uiDataBits < 5) ||
        (serial_options->uiDataBits > 8))
        return -1;

    return 0;
}

static int fb_nuttx_serial_device_path(const char *requested, char *path,
    size_t path_size)
{
    unsigned int port;
    int written;

    if ((requested == NULL) || (*requested == '\0') ||
        (path == NULL) || (path_size == 0))
        return -1;

    if (strncasecmp(requested, "COM", 3) == 0) {
        if (requested[3] == '\0')
            port = 1;
        else if (fb_nuttx_serial_parse_uint(requested + 3, &port) != 0)
            return -1;

        if (port == 0)
            return -1;

        written = snprintf(path, path_size, "/dev/ttyS%u", port - 1);
    } else {
        written = snprintf(path, path_size, "%s", requested);
    }

    if ((written < 0) || ((size_t)written >= path_size))
        return -1;

    return 0;
}

#if FB_NUTTX_HAVE_SERIAL_TERMIOS
static int fb_nuttx_serial_configure(int fd,
    const FB_SERIAL_OPTIONS *serial_options, struct termios *old_termios)
{
    struct termios configured;

    if ((serial_options == NULL) || (old_termios == NULL))
        return -1;

    if (tcgetattr(fd, old_termios) != 0)
        return -1;

    configured = *old_termios;
    cfmakeraw(&configured);
    configured.c_cflag |= CREAD;
    configured.c_cflag &= ~CSIZE;

    switch (serial_options->uiDataBits) {
    case 5:
        configured.c_cflag |= CS5;
        break;
    case 6:
        configured.c_cflag |= CS6;
        break;
    case 7:
        configured.c_cflag |= CS7;
        break;
    case 8:
        configured.c_cflag |= CS8;
        break;
    default:
        return -1;
    }

    configured.c_cflag &= ~(PARENB | PARODD | CSTOPB);
    configured.c_iflag &= ~(INPCK | IGNPAR);

    switch (serial_options->Parity) {
    case FB_SERIAL_PARITY_NONE:
        break;
    case FB_SERIAL_PARITY_EVEN:
        configured.c_cflag |= PARENB;
        break;
    case FB_SERIAL_PARITY_ODD:
        configured.c_cflag |= PARENB | PARODD;
        break;
    default:
        /* NuttX termios has no portable mark/space parity flag. */
        return -1;
    }

    if (serial_options->StopBits != FB_SERIAL_STOP_BITS_1)
        configured.c_cflag |= CSTOPB;

    if ((serial_options->StopBits == FB_SERIAL_STOP_BITS_1_5) &&
        (serial_options->uiDataBits != 5))
        return -1;

    if (serial_options->CheckParity)
        configured.c_iflag |= INPCK;

    if (serial_options->DiscardOnError || serial_options->IgnoreAllErrors)
        configured.c_iflag |= IGNPAR;

    if (serial_options->KeepDTREnabled)
        configured.c_cflag &= ~HUPCL;
    else
        configured.c_cflag |= HUPCL;

    if (serial_options->DurationDSR || serial_options->DurationCD)
        configured.c_cflag &= ~CLOCAL;
    else
        configured.c_cflag |= CLOCAL;

#ifdef CRTSCTS
    if (serial_options->DurationCTS && !serial_options->SuppressRTS)
        configured.c_cflag |= CRTSCTS;
    else
        configured.c_cflag &= ~CRTSCTS;
#else
    if (serial_options->DurationCTS && !serial_options->SuppressRTS)
        return -1;
#endif

    if (serial_options->AddLF)
        configured.c_oflag |= OPOST | ONLCR;

    configured.c_cc[VMIN] = 0;
    configured.c_cc[VTIME] = 1;

    if (cfsetspeed(&configured, (speed_t)serial_options->uiSpeed) != 0)
        return -1;

    if (tcsetattr(fd, TCSAFLUSH, &configured) != 0)
        return -1;

    return 0;
}
#endif

int32 fb_FileOpenCom(const FBSTRING *device, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 reclen, const char *encoding)
{
    FB_SERIAL_OPTIONS serial_options;
    char path[FB_NUTTX_SERIAL_PATH_MAX];
    char *device_text;
    char *options_text;
    FILE *stream;
    int fd;
    int open_flags;

    (void)mode;
    (void)access;
    (void)lock;
    (void)reclen;
    (void)encoding;

    if ((file_num <= 0) || (file_num >= FB_NUTTX_MAX_FILES)) {
        fb_nuttx_error_num = FB_RTERROR_ILLEGALFUNCTIONCALL;
        return -1;
    }

    device_text = fb_nuttx_string_to_cstr(device);

    if (device_text == NULL) {
        fb_nuttx_error_num = FB_RTERROR_OUTOFMEM;
        return -1;
    }

    options_text = strchr(device_text, ':');

    if (options_text == NULL) {
        free(device_text);
        fb_nuttx_error_num = FB_RTERROR_ILLEGALFUNCTIONCALL;
        return -1;
    }

    *options_text = '\0';
    options_text++;

    if ((fb_nuttx_serial_device_path(device_text, path, sizeof(path)) != 0) ||
        (fb_nuttx_serial_parse_options(options_text, &serial_options) != 0)) {
        free(device_text);
        fb_nuttx_error_num = FB_RTERROR_ILLEGALFUNCTIONCALL;
        return -1;
    }

    free(device_text);
    fb_FileClose(file_num);

    open_flags = O_RDWR;
#ifdef O_NOCTTY
    open_flags |= O_NOCTTY;
#endif
    fd = open(path, open_flags);

    if (fd < 0) {
        fb_nuttx_error_num = FB_RTERROR_FILENOTFOUND;
        return -1;
    }

#if FB_NUTTX_HAVE_SERIAL_TERMIOS
    if (fb_nuttx_serial_configure(fd, &serial_options,
            &fb_nuttx_serial_old_termios[file_num]) != 0) {
        close(fd);
        fb_nuttx_error_num = FB_RTERROR_FILEIO;
        return -1;
    }

    fb_nuttx_serial_old_termios_valid[file_num] = 1;
#else
    /*
        A board built without CONFIG_SERIAL_TERMIOS may still expose a UART as
        a preconfigured character device. Keep byte I/O available while making
        the absence of framing control explicit through the build option.
    */
    (void)serial_options;
#endif

    stream = fdopen(fd, "r+");

    if (stream == NULL) {
#if FB_NUTTX_HAVE_SERIAL_TERMIOS
        (void)tcsetattr(fd, TCSAFLUSH,
                       &fb_nuttx_serial_old_termios[file_num]);
        fb_nuttx_serial_old_termios_valid[file_num] = 0;
#endif
        close(fd);
        fb_nuttx_error_num = FB_RTERROR_FILEIO;
        return -1;
    }

    (void)setvbuf(stream, NULL, _IONBF, 0);
    fb_nuttx_files[file_num] = stream;
    fb_nuttx_file_kind[file_num] = FB_NUTTX_FILE_KIND_COM;
    fb_nuttx_file_record_len[file_num] = 0;
    fb_nuttx_serial_output_lines[file_num] = 0;
    fb_nuttx_error_num = FB_RTERROR_OK;

    return 0;
}

static int fb_nuttx_serial_fd(const int file_number)
{
    if ((file_number <= 0) || (file_number >= FB_NUTTX_MAX_FILES) ||
        (fb_nuttx_file_kind[file_number] != FB_NUTTX_FILE_KIND_COM) ||
        (fb_nuttx_files[file_number] == NULL))
        return -1;

    return fileno(fb_nuttx_files[file_number]);
}

static unsigned int fb_nuttx_serial_lines_from_native(int native_lines)
{
    unsigned int lines = 0;

#ifdef TIOCM_CTS
    if ((native_lines & TIOCM_CTS) != 0)
        lines |= FB_COM_LINE_CTS;
#endif
#ifdef TIOCM_DSR
    if ((native_lines & TIOCM_DSR) != 0)
        lines |= FB_COM_LINE_DSR;
#endif
#ifdef TIOCM_CAR
    if ((native_lines & TIOCM_CAR) != 0)
        lines |= FB_COM_LINE_DCD;
#elif defined(TIOCM_CD)
    if ((native_lines & TIOCM_CD) != 0)
        lines |= FB_COM_LINE_DCD;
#elif defined(TIOCM_DCD)
    if ((native_lines & TIOCM_DCD) != 0)
        lines |= FB_COM_LINE_DCD;
#endif
#ifdef TIOCM_RI
    if ((native_lines & TIOCM_RI) != 0)
        lines |= FB_COM_LINE_RI;
#endif
#ifdef TIOCM_RTS
    if ((native_lines & TIOCM_RTS) != 0)
        lines |= FB_COM_LINE_RTS;
#endif
#ifdef TIOCM_DTR
    if ((native_lines & TIOCM_DTR) != 0)
        lines |= FB_COM_LINE_DTR;
#endif

    return lines;
}

static int fb_nuttx_serial_lines_to_native(unsigned int lines)
{
    int native_lines = 0;

#ifdef TIOCM_RTS
    if ((lines & FB_COM_LINE_RTS) != 0)
        native_lines |= TIOCM_RTS;
#endif
#ifdef TIOCM_DTR
    if ((lines & FB_COM_LINE_DTR) != 0)
        native_lines |= TIOCM_DTR;
#endif

    return native_lines;
}

FBCALL int fb_ComGetStatus(int file_number, FB_COM_STATUS *status)
{
    int fd;
    int native_lines;
    int queued;

    if (status == NULL)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    memset(status, 0, sizeof(*status));
    fd = fb_nuttx_serial_fd(file_number);

    if (fd < 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    status->lines = fb_nuttx_serial_output_lines[file_number];

#if FB_NUTTX_HAVE_SERIAL_TERMIOS
    status->capabilities |= FB_COM_CAP_PURGE_RX | FB_COM_CAP_PURGE_TX;
#endif
#if defined(TIOCSBRK) && defined(TIOCCBRK)
    status->capabilities |= FB_COM_CAP_BREAK;
#endif
#ifdef TIOCMGET
    native_lines = 0;

    if (ioctl(fd, TIOCMGET, (unsigned long)(uintptr_t)&native_lines) == 0) {
        status->lines = fb_nuttx_serial_lines_from_native(native_lines);
        status->capabilities |= FB_COM_CAP_INPUT_LINES;
#if defined(TIOCMSET) && defined(TIOCM_RTS) && defined(TIOCM_DTR)
        status->capabilities |= FB_COM_CAP_OUTPUT_LINES;
#endif
    }
#else
    (void)native_lines;
#endif
#ifdef TIOCINQ
    queued = 0;

    if (ioctl(fd, TIOCINQ, (unsigned long)(uintptr_t)&queued) == 0) {
        status->rx_queued = queued > 0 ? (unsigned int)queued : 0;
        status->capabilities |= FB_COM_CAP_RX_QUEUE;
    }
#else
    (void)queued;
#endif
#ifdef TIOCOUTQ
    queued = 0;

    if (ioctl(fd, TIOCOUTQ, (unsigned long)(uintptr_t)&queued) == 0) {
        status->tx_queued = queued > 0 ? (unsigned int)queued : 0;
        status->capabilities |= FB_COM_CAP_TX_QUEUE;
    }
#endif

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

FBCALL int fb_ComSetLines(int file_number, unsigned int mask,
    unsigned int values)
{
    const unsigned int valid_lines = FB_COM_LINE_RTS | FB_COM_LINE_DTR;
    int native_lines;
    int native_mask;
    int fd;

    if ((mask == 0) || ((mask & ~valid_lines) != 0) ||
        ((values & ~valid_lines) != 0))
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    fd = fb_nuttx_serial_fd(file_number);

    if (fd < 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

#if defined(TIOCMGET) && defined(TIOCMSET) && \
    defined(TIOCM_RTS) && defined(TIOCM_DTR)
    native_lines = 0;
    native_mask = fb_nuttx_serial_lines_to_native(mask);

    if (ioctl(fd, TIOCMGET, (unsigned long)(uintptr_t)&native_lines) != 0)
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);

    native_lines &= ~native_mask;
    native_lines |= fb_nuttx_serial_lines_to_native(values & mask);

    if (ioctl(fd, TIOCMSET, (unsigned long)(uintptr_t)&native_lines) != 0)
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);

    fb_nuttx_serial_output_lines[file_number] &= ~mask;
    fb_nuttx_serial_output_lines[file_number] |= values & mask;

    return fb_ErrorSetNum(FB_RTERROR_OK);
#else
    (void)values;
    (void)native_lines;
    (void)native_mask;
    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
#endif
}

FBCALL int fb_ComSetBreak(int file_number, int enabled)
{
    int fd = fb_nuttx_serial_fd(file_number);

    if (fd < 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

#if defined(TIOCSBRK) && defined(TIOCCBRK)
    if (ioctl(fd, enabled ? TIOCSBRK : TIOCCBRK, 0ul) != 0)
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);

    return fb_ErrorSetNum(FB_RTERROR_OK);
#else
    (void)enabled;
    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
#endif
}

FBCALL int fb_ComPurge(int file_number, unsigned int queues)
{
    const unsigned int valid_queues = FB_COM_PURGE_RX | FB_COM_PURGE_TX;
    int selector;
    int fd;

    if ((queues == 0) || ((queues & ~valid_queues) != 0))
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    fd = fb_nuttx_serial_fd(file_number);

    if (fd < 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

#if FB_NUTTX_HAVE_SERIAL_TERMIOS
    if (queues == valid_queues)
        selector = TCIOFLUSH;
    else if (queues == FB_COM_PURGE_RX)
        selector = TCIFLUSH;
    else
        selector = TCOFLUSH;

    if (tcflush(fd, selector) != 0)
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);

    return fb_ErrorSetNum(FB_RTERROR_OK);
#else
    (void)selector;
    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
#endif
}

/* end of fb_nuttx_file.c */
