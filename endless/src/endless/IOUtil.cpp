#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>

#include "GeneralCode.h"

#define PERROR(_err, msg) do { \
    if (_err != 0) { \
        uprintf("%s:%d %s: %s",__FILE__, __LINE__, msg, strerror(_err)); \
        goto error; \
    } \
} while (0)

// Reads 'path', in its entirety, into 'buffer'.
// Returns the number of bytes read, or 0 if the file is larger than
// 'buffer_size' or an error occurred.
size_t ReadEntireFile(const wchar_t *path, unsigned char *buffer, size_t buffer_size) {
    FILE *fp = NULL;
    long size = 0;
    size_t size_read = 0;
    errno_t err;

    FUNCTION_ENTER_FMT("%ls", path);

    err = _wfopen_s(&fp, path, L"rb");
    PERROR(err, "_wfopen_s failed");

    err = fseek(fp, 0L, SEEK_END);
    PERROR(err, "fseek(SEEK_END) failed");

    size = ftell(fp);
    rewind(fp);

    IFFALSE_GOTOERROR(size >= 0, "ftell returned negative");
    if ((size_t) size > buffer_size) {
        uprintf("%ls is %ld bytes long; supplied buffer is only %Id bytes", path, size, buffer_size);
        goto error;
    }
    buffer_size = size;

    while (!feof(fp) && !ferror(fp) && size_read < buffer_size) {
        size_t count = fread(buffer + size_read, 1, 512, fp);
        size_read += count;
    }

    err = ferror(fp);
    if (err != 0) {
        size_read = 0;
        PERROR(err, "fread failed");
    }

error:
    safe_closefile(fp);
    return size_read;
}
