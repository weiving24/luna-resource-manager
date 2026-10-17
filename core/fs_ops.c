#include "fs_ops.h"
#include "fs.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int path_is_within(const char *parent, const char *candidate)
{
    size_t length = strlen(parent);
    if (strncmp(parent, candidate, length) != 0)
        return 0;
    if (length == 1 && parent[0] == '/')
        return candidate[0] == '/';
    return candidate[length] == '\0' || candidate[length] == '/';
}

static int destination_parent(const char *dst, char *parent, size_t size,
                              char *base, size_t base_size)
{
    const char *slash;
    const char *base_part;
    size_t parent_length;
    size_t base_length;

    if (dst == NULL || parent == NULL || base == NULL || dst[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    slash = strrchr(dst, '/');
    if (slash == NULL) {
        base_part = dst;
        parent_length = 1;
        if (size < 2) {
            errno = ENAMETOOLONG;
            return -1;
        }
        strcpy(parent, ".");
    } else {
        base_part = slash + 1;
        if (slash == dst) {
            parent_length = 1;
            if (size < 2) {
                errno = ENAMETOOLONG;
                return -1;
            }
            strcpy(parent, "/");
        } else {
            parent_length = (size_t)(slash - dst);
            if (parent_length + 1 > size) {
                errno = ENAMETOOLONG;
                return -1;
            }
            memcpy(parent, dst, parent_length);
            parent[parent_length] = '\0';
        }
    }
    (void)parent_length;
    base_length = strlen(base_part);
    if (base_length == 0 || base_length >= base_size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(base, base_part, base_length + 1);
    return 0;
}

int fs_remove_recursive(const char *path)
{
    struct stat st;

    if (path == NULL || path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    if (lstat(path, &st) != 0)
        return -1;

    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))
        return unlink(path);

    {
        char resolved[PATH_MAX];
        char current_directory[PATH_MAX];
        if (realpath(path, resolved) == NULL)
            return -1;
        if (strcmp(resolved, "/") == 0 ||
            (getcwd(current_directory, sizeof(current_directory)) != NULL &&
             path_is_within(resolved, current_directory))) {
            errno = EPERM;
            return -1;
        }
    }

    {
        DIR *directory = opendir(path);
        struct dirent *entry;
        int saved_errno = 0;
        if (directory == NULL)
            return -1;

        errno = 0;
        while ((entry = readdir(directory)) != NULL) {
            char child[PATH_MAX];
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;
            if (fs_join(child, sizeof(child), path, entry->d_name) != 0 ||
                fs_remove_recursive(child) != 0) {
                saved_errno = errno;
                break;
            }
            errno = 0;
        }
        if (entry == NULL && errno != 0 && saved_errno == 0)
            saved_errno = errno;
        if (closedir(directory) != 0 && saved_errno == 0)
            saved_errno = errno;
        if (saved_errno != 0) {
            errno = saved_errno;
            return -1;
        }
    }
    return rmdir(path);
}

static int ensure_replaceable(const char *dst, struct stat *existing, int *exists)
{
    if (lstat(dst, existing) == 0) {
        *exists = 1;
        return 0;
    }
    if (errno == ENOENT) {
        *exists = 0;
        return 0;
    }
    return -1;
}

static int copy_regular_file(const char *src, const char *dst,
                             const struct stat *source_stat)
{
    int input = -1;
    int output = -1;
    char buffer[16384];
    int result = -1;

    input = open(src, O_RDONLY);
    if (input < 0)
        return -1;
    output = open(dst, O_WRONLY | O_CREAT | O_TRUNC, source_stat->st_mode & 0777);
    if (output < 0) {
        int saved = errno;
        close(input);
        errno = saved;
        return -1;
    }

    for (;;) {
        ssize_t got = read(input, buffer, sizeof(buffer));
        if (got == 0) {
            result = 0;
            break;
        }
        if (got < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        {
            ssize_t offset = 0;
            while (offset < got) {
                ssize_t put = write(output, buffer + offset, (size_t)(got - offset));
                if (put < 0 && errno == EINTR)
                    continue;
                if (put <= 0)
                    goto done;
                offset += put;
            }
        }
    }

done:
    {
        int saved = errno;
        if (result == 0 && fchmod(output, source_stat->st_mode & 0777) != 0) {
            result = -1;
            saved = errno;
        }
        if (close(input) != 0 && result == 0) {
            result = -1;
            saved = errno;
        }
        if (close(output) != 0 && result == 0) {
            result = -1;
            saved = errno;
        }
        if (result != 0)
            errno = saved != 0 ? saved : EIO;
    }
    return result;
}

static int copy_node(const char *src, const char *dst)
{
    struct stat source_stat;
    struct stat destination_stat;
    int destination_exists = 0;

    if (lstat(src, &source_stat) != 0)
        return -1;
    if (ensure_replaceable(dst, &destination_stat, &destination_exists) != 0)
        return -1;

    if (S_ISDIR(source_stat.st_mode)) {
        DIR *directory;
        struct dirent *entry;
        int saved_errno = 0;

        if (destination_exists && !S_ISDIR(destination_stat.st_mode)) {
            if (fs_remove_recursive(dst) != 0)
                return -1;
            destination_exists = 0;
        }
        if (!destination_exists && mkdir(dst, 0700) != 0)
            return -1;
        if (chmod(dst, (source_stat.st_mode & 0777) | 0700) != 0)
            return -1;

        directory = opendir(src);
        if (directory == NULL)
            return -1;
        errno = 0;
        while ((entry = readdir(directory)) != NULL) {
            char source_child[PATH_MAX];
            char destination_child[PATH_MAX];
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
                continue;
            if (fs_join(source_child, sizeof(source_child), src, entry->d_name) != 0 ||
                fs_join(destination_child, sizeof(destination_child), dst, entry->d_name) != 0 ||
                copy_node(source_child, destination_child) != 0) {
                saved_errno = errno;
                break;
            }
            errno = 0;
        }
        if (entry == NULL && errno != 0 && saved_errno == 0)
            saved_errno = errno;
        if (closedir(directory) != 0 && saved_errno == 0)
            saved_errno = errno;
        if (saved_errno != 0) {
            errno = saved_errno;
            return -1;
        }
        return chmod(dst, source_stat.st_mode & 0777);
    }

    if (S_ISREG(source_stat.st_mode)) {
        if (destination_exists) {
            if (S_ISDIR(destination_stat.st_mode)) {
                if (fs_remove_recursive(dst) != 0)
                    return -1;
            } else if (S_ISLNK(destination_stat.st_mode)) {
                if (unlink(dst) != 0)
                    return -1;
            }
        }
        return copy_regular_file(src, dst, &source_stat);
    }

    if (S_ISLNK(source_stat.st_mode)) {
        char target[PATH_MAX];
        ssize_t length = readlink(src, target, sizeof(target) - 1);
        if (length < 0)
            return -1;
        if ((size_t)length >= sizeof(target) - 1) {
            errno = ENAMETOOLONG;
            return -1;
        }
        target[length] = '\0';
        if (destination_exists && fs_remove_recursive(dst) != 0)
            return -1;
        return symlink(target, dst);
    }

    errno = ENOTSUP;
    return -1;
}

int fs_copy(const char *src, const char *dst)
{
    struct stat source_stat;
    struct stat source_follow_stat;
    char source_real[PATH_MAX];
    char parent_path[PATH_MAX];
    char base_name[PATH_MAX];
    char parent_real[PATH_MAX];
    char destination_real[PATH_MAX];

    if (src == NULL || dst == NULL || src[0] == '\0' || dst[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    if (lstat(src, &source_stat) != 0)
        return -1;
    if (realpath(src, source_real) == NULL)
        return -1;
    if (realpath(dst, destination_real) != NULL) {
        struct stat destination_stat;
        if (strcmp(source_real, destination_real) == 0) {
            errno = EINVAL;
            return -1;
        }
        if (stat(src, &source_follow_stat) == 0 && stat(dst, &destination_stat) == 0 &&
            source_follow_stat.st_dev == destination_stat.st_dev &&
            source_follow_stat.st_ino == destination_stat.st_ino) {
            errno = EINVAL;
            return -1;
        }
    }

    if (S_ISDIR(source_stat.st_mode)) {
        if (destination_parent(dst, parent_path, sizeof(parent_path),
                               base_name, sizeof(base_name)) != 0)
            return -1;
        if (realpath(parent_path, parent_real) == NULL)
            return -1;
        if (path_is_within(source_real, parent_real)) {
            errno = EINVAL;
            return -1;
        }
    }
    return copy_node(src, dst);
}

int fs_move(const char *src, const char *dst)
{
    if (src == NULL || dst == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (rename(src, dst) == 0)
        return 0;
    if (errno != EXDEV)
        return -1;
    if (fs_copy(src, dst) != 0)
        return -1;
    if (fs_remove_recursive(src) != 0)
        return -1;
    return 0;
}

int fs_make_directory(const char *path, unsigned int mode)
{
    if (path == NULL || path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    return mkdir(path, (mode_t)mode);
}
