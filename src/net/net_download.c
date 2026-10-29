#define _POSIX_C_SOURCE 200809L
#include "net.h"
#include "crypto/sha256.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool s_net_initialized = false;

bool net_init(void) {
    if (!s_net_initialized) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            return false;
        }
        s_net_initialized = true;
    }
    return true;
}

void net_cleanup(void) {
    if (s_net_initialized) {
        curl_global_cleanup();
        s_net_initialized = false;
    }
}

static size_t write_file_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *fp = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, fp);
}

bool net_download_file(const char *url, const char *dest_path, const char *expected_sha256, char *err_buf, size_t err_size) {
    if (!net_init()) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to initialize network layer");
        return false;
    }

    char part_path[1024];
    snprintf(part_path, sizeof(part_path), "%s.part", dest_path);

    FILE *fp = fopen(part_path, "wb");
    if (!fp) {
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to open destination: %s", part_path);
        return false;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        unlink(part_path);
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to initialize curl handle");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        unlink(part_path);
        if (err_buf && err_size > 0) {
            snprintf(err_buf, err_size, "Download failed for %s: %s (status %ld)", url, curl_easy_strerror(res), http_code);
        }
        return false;
    }

    if (expected_sha256 && *expected_sha256) {
        char actual_sha256[65];
        if (!sha256_file(part_path, actual_sha256) || strcmp(actual_sha256, expected_sha256) != 0) {
            unlink(part_path);
            if (err_buf && err_size > 0) {
                snprintf(err_buf, err_size, "SHA256 mismatch: expected %s, got %s", expected_sha256, actual_sha256);
            }
            return false;
        }
    }

    if (rename(part_path, dest_path) != 0) {
        unlink(part_path);
        if (err_buf && err_size > 0) snprintf(err_buf, err_size, "Failed to rename temp file to %s", dest_path);
        return false;
    }

    return true;
}

typedef struct {
    DownloadTask *task;
    FILE *fp;
    char part_path[1024];
    CURL *easy;
} TaskContext;

bool net_download_batch(DownloadTask *tasks, size_t count, int max_concurrent) {
    if (count == 0) return true;
    if (!net_init()) return false;
    if (max_concurrent <= 0) max_concurrent = 4;

    CURLM *multi = curl_multi_init();
    if (!multi) return false;

    TaskContext *contexts = (TaskContext *)calloc(count, sizeof(TaskContext));
    size_t next_task = 0;
    size_t active_handles = 0;

    while (next_task < count && (int)active_handles < max_concurrent) {
        DownloadTask *t = &tasks[next_task];
        snprintf(contexts[next_task].part_path, sizeof(contexts[next_task].part_path), "%s.part", t->dest_path);
        contexts[next_task].fp = fopen(contexts[next_task].part_path, "wb");
        if (contexts[next_task].fp) {
            CURL *easy = curl_easy_init();
            contexts[next_task].easy = easy;
            contexts[next_task].task = t;
            curl_easy_setopt(easy, CURLOPT_URL, t->url);
            curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_file_cb);
            curl_easy_setopt(easy, CURLOPT_WRITEDATA, contexts[next_task].fp);
            curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
            curl_easy_setopt(easy, CURLOPT_PRIVATE, &contexts[next_task]);
            curl_multi_add_handle(multi, easy);
            active_handles++;
        } else {
            t->success = false;
            snprintf(t->error_msg, sizeof(t->error_msg), "Failed to open part file");
        }
        next_task++;
    }

    int still_running = 1;
    while (still_running || active_handles > 0) {
        curl_multi_perform(multi, &still_running);

        int msgs_left = 0;
        CURLMsg *msg = NULL;
        while ((msg = curl_multi_info_read(multi, &msgs_left)) != NULL) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *easy = msg->easy_handle;
                TaskContext *ctx = NULL;
                curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ctx);

                if (ctx) {
                    fclose(ctx->fp);
                    ctx->fp = NULL;

                    long http_code = 0;
                    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &http_code);
                    ctx->task->http_status = (int)http_code;

                    if (msg->data.result == CURLE_OK) {
                        bool verified = true;
                        if (ctx->task->expected_sha256 && *ctx->task->expected_sha256) {
                            char actual[65];
                            if (!sha256_file(ctx->part_path, actual) || strcmp(actual, ctx->task->expected_sha256) != 0) {
                                verified = false;
                                snprintf(ctx->task->error_msg, sizeof(ctx->task->error_msg), "Checksum mismatch");
                            }
                        }

                        if (verified && rename(ctx->part_path, ctx->task->dest_path) == 0) {
                            ctx->task->success = true;
                        } else {
                            unlink(ctx->part_path);
                            ctx->task->success = false;
                        }
                    } else {
                        unlink(ctx->part_path);
                        ctx->task->success = false;
                        snprintf(ctx->task->error_msg, sizeof(ctx->task->error_msg), "%s", curl_easy_strerror(msg->data.result));
                    }
                }

                curl_multi_remove_handle(multi, easy);
                curl_easy_cleanup(easy);
                active_handles--;

                if (next_task < count) {
                    DownloadTask *t = &tasks[next_task];
                    snprintf(contexts[next_task].part_path, sizeof(contexts[next_task].part_path), "%s.part", t->dest_path);
                    contexts[next_task].fp = fopen(contexts[next_task].part_path, "wb");
                    if (contexts[next_task].fp) {
                        CURL *next_easy = curl_easy_init();
                        contexts[next_task].easy = next_easy;
                        contexts[next_task].task = t;
                        curl_easy_setopt(next_easy, CURLOPT_URL, t->url);
                        curl_easy_setopt(next_easy, CURLOPT_WRITEFUNCTION, write_file_cb);
                        curl_easy_setopt(next_easy, CURLOPT_WRITEDATA, contexts[next_task].fp);
                        curl_easy_setopt(next_easy, CURLOPT_FOLLOWLOCATION, 1L);
                        curl_easy_setopt(next_easy, CURLOPT_FAILONERROR, 1L);
                        curl_easy_setopt(next_easy, CURLOPT_PRIVATE, &contexts[next_task]);
                        curl_multi_add_handle(multi, next_easy);
                        active_handles++;
                    }
                    next_task++;
                }
            }
        }

        if (still_running) {
            curl_multi_poll(multi, NULL, 0, 100, NULL);
        }
    }

    curl_multi_cleanup(multi);
    free(contexts);

    bool all_ok = true;
    for (size_t i = 0; i < count; ++i) {
        if (!tasks[i].success) all_ok = false;
    }
    return all_ok;
}
