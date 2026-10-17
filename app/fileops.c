#include "fileops.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "dialogs.h"
#include "dir_picker.h"
#include "fs.h"
#include "fs_ops.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char *base_name(const char *path)
{
    const char *slash;
    if (path == NULL)
        return "";
    slash = strrchr(path, '/');
    if (slash == NULL)
        return path;
    return slash[1] != '\0' ? slash + 1 : path;
}

static void set_result(AppState *state, const char *operation, int result)
{
    if (state == NULL)
        return;
    if (result == 0)
        (void)snprintf(state->status, sizeof(state->status), "%s succeeded", operation);
    else
        (void)snprintf(state->status, sizeof(state->status), "%s failed: %s",
                       operation, strerror(errno));
}

static int target_path(const char *directory, const char *source, char *target, size_t size)
{
    return fs_join(target, size, directory, base_name(source));
}

static int approve_overwrite(const char *target)
{
    struct stat st;
    char message[PATH_MAX + 64];
    if (lstat(target, &st) != 0)
        return errno == ENOENT;
    (void)snprintf(message, sizeof(message), "Target already exists:\n%s\n\nOverwrite it?", target);
    return dialog_confirm("Confirm Overwrite", message);
}

void fileops_copy(AppState *state, const char *src, const char *start_dir)
{
    char destination_directory[PATH_MAX];
    char destination[PATH_MAX];
    int result;

    if (src == NULL || dir_picker_select(start_dir, destination_directory,
                                         sizeof(destination_directory)) != 0)
        return;
    if (target_path(destination_directory, src, destination, sizeof(destination)) != 0) {
        errno = ENAMETOOLONG;
        set_result(state, "Copy", -1);
        return;
    }
    if (strcmp(src, destination) == 0) {
        errno = EINVAL;
        set_result(state, "Copy", -1);
        return;
    }
    if (!approve_overwrite(destination))
        return;
    result = fs_copy(src, destination);
    set_result(state, "Copy", result);
}

void fileops_move(AppState *state, const char *src, const char *start_dir)
{
    char destination_directory[PATH_MAX];
    char destination[PATH_MAX];
    int result;

    if (src == NULL || dir_picker_select(start_dir, destination_directory,
                                         sizeof(destination_directory)) != 0)
        return;
    if (target_path(destination_directory, src, destination, sizeof(destination)) != 0) {
        errno = ENAMETOOLONG;
        set_result(state, "Move", -1);
        return;
    }
    if (strcmp(src, destination) == 0) {
        errno = EINVAL;
        set_result(state, "Move", -1);
        return;
    }
    if (!approve_overwrite(destination))
        return;
    result = fs_move(src, destination);
    set_result(state, "Move", result);
}

void fileops_delete(AppState *state, const char *path)
{
    char message[PATH_MAX + 64];
    int result;
    if (path == NULL)
        return;
    (void)snprintf(message, sizeof(message), "Delete this item?\n%s\n\nThis cannot be undone.", path);
    if (!dialog_confirm("Confirm Delete", message))
        return;
    result = fs_remove_recursive(path);
    set_result(state, "Delete", result);
}

void fileops_rename(AppState *state, const char *source)
{
    char new_name[PATH_MAX];
    char target[PATH_MAX];
    char parent[PATH_MAX];
    const char *slash;
    size_t length;
    int result;

    if (source == NULL)
        return;
    (void)snprintf(new_name, sizeof(new_name), "%s", base_name(source));
    if (dialog_input("Rename", "New name (without slash):", new_name, sizeof(new_name)) != 0 ||
        new_name[0] == '\0')
        return;
    if (strchr(new_name, '/') != NULL || strcmp(new_name, ".") == 0 || strcmp(new_name, "..") == 0) {
        errno = EINVAL;
        set_result(state, "Rename", -1);
        return;
    }
    slash = strrchr(source, '/');
    if (slash == NULL) {
        (void)snprintf(parent, sizeof(parent), ".");
    } else if (slash == source) {
        (void)snprintf(parent, sizeof(parent), "/");
    } else {
        length = (size_t)(slash - source);
        if (length >= sizeof(parent)) {
            errno = ENAMETOOLONG;
            set_result(state, "Rename", -1);
            return;
        }
        memcpy(parent, source, length);
        parent[length] = '\0';
    }
    if (fs_join(target, sizeof(target), parent, new_name) != 0) {
        set_result(state, "Rename", -1);
        return;
    }
    if (strcmp(source, target) != 0 && !approve_overwrite(target))
        return;
    result = rename(source, target);
    set_result(state, "Rename", result);
}

void fileops_newdir(AppState *state, const char *directory)
{
    char name[PATH_MAX] = "newdir";
    char target[PATH_MAX];
    int result;
    if (directory == NULL ||
        dialog_input("New Directory", "Directory name (without slash):", name, sizeof(name)) != 0 ||
        name[0] == '\0')
        return;
    if (strchr(name, '/') != NULL || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        errno = EINVAL;
        set_result(state, "Create directory", -1);
        return;
    }
    if (fs_join(target, sizeof(target), directory, name) != 0) {
        set_result(state, "Create directory", -1);
        return;
    }
    result = fs_make_directory(target, 0755);
    set_result(state, "Create directory", result);
}
