#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "archive_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARCHIVE_EOF       1
#define ARCHIVE_OK        0
#define ARCHIVE_RETRY   (-10)
#define ARCHIVE_WARN    (-20)
#define ARCHIVE_FAILED  (-25)
#define ARCHIVE_FATAL   (-30)

struct archive;

const char *archive_version_details(void);
const char *archive_error_string(struct archive *);
int archive_errno(struct archive *);

struct archive *archive_read_new(void);
int archive_read_support_filter_all(struct archive *);
int archive_read_support_filter_zstd(struct archive *);
int archive_read_support_format_all(struct archive *);
int archive_read_support_format_tar(struct archive *);
int archive_read_open_filename(struct archive *, const char *filename, size_t block_size);
int archive_read_next_header(struct archive *, struct archive_entry **);
int64_t archive_read_data(struct archive *, void *, size_t);
int archive_read_data_block(struct archive *, const void **buff, size_t *size, int64_t *offset);
int archive_read_data_skip(struct archive *);
int archive_read_close(struct archive *);
int archive_read_free(struct archive *);

struct archive *archive_write_new(void);
int archive_write_add_filter_zstd(struct archive *);
int archive_write_add_filter_gzip(struct archive *);
int archive_write_set_format_pax_restricted(struct archive *);
int archive_write_set_format_ustar(struct archive *);
int archive_write_open_filename(struct archive *, const char *filename);
int archive_write_header(struct archive *, struct archive_entry *);
int64_t archive_write_data(struct archive *, const void *, size_t);
int archive_write_close(struct archive *);
int archive_write_free(struct archive *);

struct archive *archive_write_disk_new(void);
int archive_write_disk_set_options(struct archive *, int flags);
#define ARCHIVE_EXTRACT_TIME          (0x0004)
#define ARCHIVE_EXTRACT_PERM          (0x0002)
#define ARCHIVE_EXTRACT_UNLINK        (0x0020)
#define ARCHIVE_EXTRACT_SECURE_SYMLINKS (0x0100)
#define ARCHIVE_EXTRACT_SECURE_NODOTDOT (0x0200)

#ifdef __cplusplus
}
#endif

#endif
