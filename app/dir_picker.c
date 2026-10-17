#include "dir_picker.h"

#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fs.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char **names;
    size_t count;
    size_t capacity;
} DirectoryNames;

static void names_free(DirectoryNames *names)
{
    size_t i;
    for (i = 0; i < names->count; ++i)
        free(names->names[i]);
    free(names->names);
    memset(names, 0, sizeof(*names));
}

static int names_add(DirectoryNames *names, const char *name)
{
    char **grown;
    char *copy;
    if (names->count == names->capacity) {
        size_t next = names->capacity == 0 ? 32 : names->capacity * 2;
        if (next < names->capacity || next > ((size_t)-1) / sizeof(*names->names)) {
            errno = ENOMEM;
            return -1;
        }
        grown = realloc(names->names, next * sizeof(*names->names));
        if (grown == NULL)
            return -1;
        names->names = grown;
        names->capacity = next;
    }
    copy = strdup(name);
    if (copy == NULL)
        return -1;
    names->names[names->count++] = copy;
    return 0;
}

static int compare_names(const void *left, const void *right)
{
    const char *const *a = left;
    const char *const *b = right;
    return strcmp(*a, *b);
}

static int read_directory_names(const char *path, DirectoryNames *names)
{
    DIR *directory;
    struct dirent *entry;
    int saved_errno = 0;

    memset(names, 0, sizeof(*names));
    if (strcmp(path, "/") != 0 && names_add(names, "..") != 0)
        return -1;
    directory = opendir(path);
    if (directory == NULL) {
        names_free(names);
        return -1;
    }
    errno = 0;
    while ((entry = readdir(directory)) != NULL) {
        char full_path[PATH_MAX];
        struct stat st;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (fs_join(full_path, sizeof(full_path), path, entry->d_name) != 0) {
            saved_errno = errno;
            break;
        }
        if (stat(full_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            errno = 0;
            continue;
        }
        if (names_add(names, entry->d_name) != 0) {
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
        names_free(names);
        errno = saved_errno;
        return -1;
    }
    if (names->count > 1) {
        size_t offset = strcmp(names->names[0], "..") == 0 ? 1 : 0;
        qsort(names->names + offset, names->count - offset,
              sizeof(*names->names), compare_names);
    }
    return 0;
}

int dir_picker_select(const char *start_directory, char *output, size_t output_size)
{
    char current[PATH_MAX];
    int selected = 0;
    int scroll = 0;

    if (output == NULL || output_size == 0) {
        errno = EINVAL;
        return -1;
    }
    if (realpath(start_directory != NULL ? start_directory : ".", current) == NULL)
        return -1;

    for (;;) {
        DirectoryNames names;
        int rows, columns, height, width, y, x, body_height;
        WINDOW *window;
        int key;
        size_t i;

        if (read_directory_names(current, &names) != 0)
            return -1;
        getmaxyx(stdscr, rows, columns);
        if (rows < 5 || columns < 12) {
            names_free(&names);
            errno = ENOSPC;
            return -1;
        }
        height = rows * 3 / 4;
        if (height < 5)
            height = 5;
        if (height > rows - 1)
            height = rows - 1;
        width = columns * 4 / 5;
        if (width < 12)
            width = columns;
        if (width > columns - 1)
            width = columns - 1;
        y = (rows - height) / 2;
        x = (columns - width) / 2;
        body_height = height - 4;
        if (selected >= (int)names.count)
            selected = names.count > 0 ? (int)names.count - 1 : 0;
        if (selected < scroll)
            scroll = selected;
        if (selected >= scroll + body_height)
            scroll = selected - body_height + 1;
        if (scroll < 0)
            scroll = 0;

        window = newwin(height, width, y, x);
        if (window == NULL) {
            names_free(&names);
            return -1;
        }
        (void)keypad(window, TRUE);
        (void)werase(window);
        (void)box(window, 0, 0);
        (void)wattrset(window, A_BOLD);
        (void)mvwprintw(window, 0, 2, " Select Directory ");
        (void)wattrset(window, A_NORMAL);
        (void)mvwaddnstr(window, 1, 2, current, width - 4);
        for (i = 0; i < (size_t)body_height && (size_t)(scroll + (int)i) < names.count; ++i) {
            int index = scroll + (int)i;
            (void)wattrset(window, index == selected ? A_REVERSE | A_BOLD : A_NORMAL);
            (void)mvwprintw(window, 2 + (int)i, 3, "[D] %.*s", width - 8,
                            names.names[index]);
        }
        (void)wattrset(window, A_REVERSE);
        (void)mvwhline(window, height - 1, 1, ' ', width - 2);
        (void)mvwprintw(window, height - 1, 2, " Up/Down: Select Enter: Enter Space: Choose Esc: Cancel ");
        (void)wattrset(window, A_NORMAL);
        (void)wrefresh(window);
        key = wgetch(window);
        (void)delwin(window);
        names_free(&names);
        (void)touchwin(stdscr);
        (void)refresh();

        if (key == 27)
            return -1;
        if (key == KEY_UP && selected > 0)
            --selected;
        else if (key == KEY_DOWN && selected + 1 < (int)names.count)
            ++selected;
        else if (key == KEY_PPAGE) {
            selected -= body_height;
            if (selected < 0)
                selected = 0;
        } else if (key == KEY_NPAGE) {
            selected += body_height;
            if (selected >= (int)names.count)
                selected = names.count > 0 ? (int)names.count - 1 : 0;
        } else if (key == ' ') {
            if (strlen(current) + 1 > output_size) {
                errno = ENAMETOOLONG;
                return -1;
            }
            strcpy(output, current);
            return 0;
        } else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            DirectoryNames next_names;
            if (read_directory_names(current, &next_names) != 0)
                return -1;
            if (selected < 0 || selected >= (int)next_names.count) {
                names_free(&next_names);
                selected = 0;
                continue;
            }
            if (strcmp(next_names.names[selected], "..") == 0) {
                if (strcmp(current, "/") != 0) {
                    char *slash = strrchr(current, '/');
                    if (slash == current)
                        current[1] = '\0';
                    else if (slash != NULL)
                        *slash = '\0';
                }
            } else {
                char next_path[PATH_MAX];
                if (fs_join(next_path, sizeof(next_path), current,
                            next_names.names[selected]) != 0) {
                    names_free(&next_names);
                    return -1;
                }
                if (strlen(next_path) >= sizeof(current)) {
                    names_free(&next_names);
                    errno = ENAMETOOLONG;
                    return -1;
                }
                strcpy(current, next_path);
            }
            names_free(&next_names);
            selected = 0;
            scroll = 0;
        }
    }
}
