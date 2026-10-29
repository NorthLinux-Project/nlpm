#ifndef ARCHIVE_ENTRY_H
#define ARCHIVE_ENTRY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct archive_entry;

#define AE_IFMT   0170000
#define AE_IFREG  0100000
#define AE_IFLNK  0120000
#define AE_IFSOCK 0140000
#define AE_IFCHR  0020000
#define AE_IFBLK  0060000
#define AE_IFDIR  0040000
#define AE_IFIFO  0010000

struct archive_entry *archive_entry_new(void);
struct archive_entry *archive_entry_clone(struct archive_entry *);
void archive_entry_free(struct archive_entry *);

const char *archive_entry_pathname(struct archive_entry *);
const char *archive_entry_symlink(struct archive_entry *);
int64_t archive_entry_size(struct archive_entry *);
mode_t archive_entry_mode(struct archive_entry *);
mode_t archive_entry_filetype(struct archive_entry *);
mode_t archive_entry_perm(struct archive_entry *);
time_t archive_entry_mtime(struct archive_entry *);

void archive_entry_set_pathname(struct archive_entry *, const char *);
void archive_entry_set_symlink(struct archive_entry *, const char *);
void archive_entry_set_size(struct archive_entry *, int64_t);
void archive_entry_set_filetype(struct archive_entry *, unsigned int);
void archive_entry_set_perm(struct archive_entry *, mode_t);
void archive_entry_set_mode(struct archive_entry *, mode_t);
void archive_entry_set_mtime(struct archive_entry *, time_t, long);

#ifdef __cplusplus
}
#endif

#endif
