#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/fs.h"
#include "core/fs_ops.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void fail(const char *message)
{
    (void)fprintf(stderr, "test_fs: %s: %s\n", message, strerror(errno));
    exit(1);
}

static void require(int condition, const char *message)
{
    if (!condition) {
        errno = errno != 0 ? errno : EINVAL;
        fail(message);
    }
}

static void write_file(const char *path, const char *text)
{
    FILE *stream = fopen(path, "w");
    if (stream == NULL)
        fail("fopen for write");
    if (fputs(text, stream) == EOF || fclose(stream) != 0)
        fail("write test file");
}

static int file_equals(const char *path, const char *expected)
{
    char buffer[256];
    FILE *stream = fopen(path, "r");
    size_t length;
    if (stream == NULL)
        return 0;
    length = fread(buffer, 1, sizeof(buffer) - 1, stream);
    if (ferror(stream)) {
        fclose(stream);
        return 0;
    }
    buffer[length] = '\0';
    fclose(stream);
    return strcmp(buffer, expected) == 0;
}

int main(void)
{
    char root[] = "/tmp/luna-resource-manager-test.XXXXXX";
    char source[PATH_MAX], destination[PATH_MAX], source_file[PATH_MAX];
    char copied_file[PATH_MAX], moved_file[PATH_MAX], external_file[PATH_MAX];
    char link_file[PATH_MAX], copied_link[PATH_MAX], child_copy[PATH_MAX];
    char joined[PATH_MAX];
    StpListItem *items = NULL;
    int count = 0;
    ssize_t link_length;
    char link_target[PATH_MAX];

    errno = 0;
    require(fs_remove_recursive("/") == -1 && errno == EPERM,
            "refuse to remove filesystem root");
    errno = 0;
    require(fs_remove_recursive(".") == -1 && errno == EPERM,
            "refuse to remove current directory");
    require(mkdtemp(root) != NULL, "mkdtemp");
    require(fs_join(source, sizeof(source), root, "source dir") == 0, "join source");
    require(fs_join(destination, sizeof(destination), root, "destination dir") == 0, "join destination");
    require(mkdir(source, 0755) == 0, "mkdir source");
    require(mkdir(destination, 0755) == 0, "mkdir destination");
    require(fs_join(source_file, sizeof(source_file), source, "quote ' $() file.txt") == 0, "join source file");
    require(fs_join(copied_file, sizeof(copied_file), destination, "quote ' $() file.txt") == 0, "join copied file");
    require(fs_join(moved_file, sizeof(moved_file), destination, "moved file.txt") == 0, "join moved file");
    require(fs_join(external_file, sizeof(external_file), root, "keep me.txt") == 0, "join external file");
    require(fs_join(link_file, sizeof(link_file), source, "link-to-outside") == 0, "join source link");
    require(fs_join(copied_link, sizeof(copied_link), destination, "link-to-outside") == 0, "join copied link");
    require(fs_join(child_copy, sizeof(child_copy), source, "nested-copy") == 0, "join nested copy");

    write_file(source_file, "shell characters must be literal\n");
    write_file(external_file, "outside target must survive\n");
    require(symlink(external_file, link_file) == 0, "create symlink");

    require(fs_list_dir(source, &items, &count) == 0, "list source directory");
    require(count == 3, "directory listing count");
    fs_free_items(items, count);

    require(fs_copy(source, destination) == 0, "copy source tree");
    require(file_equals(copied_file, "shell characters must be literal\n"), "copied contents");
    link_length = readlink(copied_link, link_target, sizeof(link_target) - 1);
    require(link_length >= 0, "copied symlink exists");
    link_target[link_length] = '\0';
    require(strcmp(link_target, external_file) == 0, "copied symlink target");
    require(fs_remove_recursive(copied_link) == 0, "remove copied symlink");
    require(file_equals(external_file, "outside target must survive\n"), "symlink target not removed");

    errno = 0;
    require(fs_copy(source, child_copy) == -1 && errno == EINVAL,
            "prevent copying directory into itself");
    require(fs_move(source_file, moved_file) == 0, "move file");
    require(access(source_file, F_OK) != 0 && errno == ENOENT, "source removed after move");
    require(file_equals(moved_file, "shell characters must be literal\n"), "moved contents");
    require(fs_remove_recursive(destination) == 0, "remove destination tree");
    require(fs_remove_recursive(source) == 0, "remove source tree");
    require(file_equals(external_file, "outside target must survive\n"), "external target remains");
    require(unlink(external_file) == 0, "remove external file");
    require(rmdir(root) == 0, "remove test directory");
    require(fs_join(joined, sizeof(joined), "/", "name") == 0 &&
            strcmp(joined, "/name") == 0, "join root path");
    (void)puts("test_fs: all tests passed");
    return 0;
}
