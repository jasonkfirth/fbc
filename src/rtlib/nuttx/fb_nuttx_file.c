/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_file.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

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

int32 fb_FileOpenCom(const FBSTRING *device, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 reclen, const char *encoding)
{
    /*
        NuttX boards expose serial devices through board-specific paths such
        as /dev/ttyS0 rather than DOS-style COM1: names.  Until there is a
        real mapping layer, report a normal open failure so BASIC programs can
        use ERR to choose a fallback path.
    */
    (void)device;
    (void)mode;
    (void)access;
    (void)lock;
    (void)file_num;
    (void)reclen;
    (void)encoding;

    fb_nuttx_error_num = 2;

    return -1;
}

/* ------------------------------------------------------------------------- */
/* DATA/READ support                                                         */
/* ------------------------------------------------------------------------- */
