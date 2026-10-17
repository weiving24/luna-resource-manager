#include "fs.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int compare_items(const void *left, const void *right)
{
    const StpListItem *a = (const StpListItem *)left;
    const StpListItem *b = (const StpListItem *)right;

    if (a->is_dir != b->is_dir)
        return b->is_dir - a->is_dir;
    return strcmp(a->text != NULL ? a->text : "",
                  b->text != NULL ? b->text : "");
}

int fs_join(char *out, size_t out_size, const char *base, const char *name)
{
    size_t base_len;
    int written;

    if (out == NULL || out_size == 0 || name == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (base == NULL || base[0] == '\0')
        base = ".";

    base_len = strlen(base);
    if (base_len > 0 && base[base_len - 1] == '/')
        written = snprintf(out, out_size, "%s%s", base, name);
    else
        written = snprintf(out, out_size, "%s/%s", base, name);

    if (written < 0 || (size_t)written >= out_size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

int fs_is_dir(const char *path)
{
    struct stat st;

    if (path == NULL || stat(path, &st) != 0)
        return 0;
    return S_ISDIR(st.st_mode) != 0;
}

int fs_can_access(const char *path)
{
    if (path == NULL) {
        errno = EINVAL;
        return 0;
    }
    return access(path, R_OK | X_OK) == 0;
}

void fs_free_items(StpListItem *items, int count)
{
    int i;

    if (items == NULL)
        return;
    for (i = 0; i < count; ++i) {
        free(items[i].text);
        free(items[i].meta);
    }
    free(items);
}

int fs_list_dir(const char *path, StpListItem **items, int *count)
{
    DIR *directory;
    struct dirent *entry;
    StpListItem *result = NULL;
    size_t used = 0;
    size_t capacity = 0;
    int saved_errno = 0;

    if (path == NULL || items == NULL || count == NULL) {
        errno = EINVAL;
        return -1;
    }
    *items = NULL;
    *count = 0;

    directory = opendir(path);
    if (directory == NULL)
        return -1;

    errno = 0;
    while ((entry = readdir(directory)) != NULL) {
        StpListItem item;
        struct stat st;
        char full_path[PATH_MAX];

        if (strcmp(entry->d_name, ".") == 0)
            continue;
        if (fs_join(full_path, sizeof(full_path), path, entry->d_name) != 0) {
            saved_errno = errno;
            break;
        }

        memset(&item, 0, sizeof(item));
        item.text = strdup(entry->d_name);
        if (item.text == NULL) {
            saved_errno = errno != 0 ? errno : ENOMEM;
            break;
        }
        if (stat(full_path, &st) == 0) {
            item.is_dir = S_ISDIR(st.st_mode) != 0;
            item.size = (int64_t)st.st_size;
            item.mtime = (int64_t)st.st_mtime;
        }

        if (used == capacity) {
            size_t next_capacity = capacity == 0 ? 32 : capacity * 2;
            StpListItem *grown;
            if (next_capacity < capacity ||
                next_capacity > ((size_t)-1) / sizeof(*result)) {
                free(item.text);
                saved_errno = ENOMEM;
                break;
            }
            grown = realloc(result, next_capacity * sizeof(*result));
            if (grown == NULL) {
                free(item.text);
                saved_errno = errno != 0 ? errno : ENOMEM;
                break;
            }
            result = grown;
            capacity = next_capacity;
        }
        result[used++] = item;
        errno = 0;
    }
    if (entry == NULL && errno != 0 && saved_errno == 0)
        saved_errno = errno;
    if (closedir(directory) != 0 && saved_errno == 0)
        saved_errno = errno;

    if (saved_errno != 0) {
        fs_free_items(result, (int)used);
        errno = saved_errno;
        return -1;
    }

    if (used > 1)
        qsort(result, used, sizeof(*result), compare_items);
    if (used > (size_t)2147483647) {
        fs_free_items(result, (int)used);
        errno = EOVERFLOW;
        return -1;
    }
    *items = result;
    *count = (int)used;
    return 0;
}

int fs_read_preview(const char *path, char **buf, int *len)
{
    FILE *stream;
    char *data;
    size_t used;
    int read_error;

    if (path == NULL || buf == NULL || len == NULL) {
        errno = EINVAL;
        return -1;
    }
    *buf = NULL;
    *len = 0;

    stream = fopen(path, "rb");
    if (stream == NULL)
        return -1;
    data = malloc(4097);
    if (data == NULL) {
        fclose(stream);
        return -1;
    }

    used = fread(data, 1, 4096, stream);
    read_error = ferror(stream);
    if (fclose(stream) != 0 && !read_error)
        read_error = 1;
    if (read_error) {
        int saved_errno = errno != 0 ? errno : EIO;
        free(data);
        errno = saved_errno;
        return -1;
    }
    data[used] = '\0';
    *buf = data;
    *len = (int)used;
    return 0;
}
