#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char *url;
    char *dest_path;
    char *expected_sha256;
    bool success;
    int http_status;
    char error_msg[256];
} DownloadTask;

bool net_init(void);
void net_cleanup(void);

bool net_download_file(const char *url, const char *dest_path, const char *expected_sha256, char *err_buf, size_t err_size);
bool net_download_batch(DownloadTask *tasks, size_t count, int max_concurrent);

#endif
