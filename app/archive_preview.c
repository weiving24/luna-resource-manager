#include "archive_preview.h"
#include "dialogs.h"

#include <archive.h>
#include <archive_entry.h>
#include <curses.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct archive_item {
    char *name;
    uint64_t size;
    int directory;
};

static int has_suffix(const char *path, const char *suffix)
{
    size_t path_len, suffix_len;
    if (path == NULL || suffix == NULL)
        return 0;
    path_len = strlen(path);
    suffix_len = strlen(suffix);
    if (path_len < suffix_len)
        return 0;
    path += path_len - suffix_len;
    while (*path != '\0' && *suffix != '\0') {
        char a = *path++;
        char b = *suffix++;
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return *path == '\0' && *suffix == '\0';
}

int archive_preview_is_supported(const char *path)
{
    static const char *const suffixes[] = {
        ".zip", ".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2",
        ".tar.xz", ".txz", ".7z"
    };
    size_t i;
    for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i)
        if (has_suffix(path, suffixes[i]))
            return 1;
    return 0;
}

static void free_items(struct archive_item *items, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        free(items[i].name);
    free(items);
}

static int append_item(struct archive_item **items, size_t *count,
                       size_t *capacity, struct archive_entry *entry)
{
    struct archive_item *grown;
    const char *name = archive_entry_pathname(entry);
    size_t next;
    if (name == NULL)
        name = "(unnamed)";
    if (*count == *capacity) {
        next = *capacity == 0 ? 64 : *capacity * 2;
        if (next < *capacity || next > 1000000) {
            errno = EFBIG;
            return -1;
        }
        grown = realloc(*items, next * sizeof(**items));
        if (grown == NULL)
            return -1;
        *items = grown;
        *capacity = next;
    }
    (*items)[*count].name = strdup(name);
    if ((*items)[*count].name == NULL)
        return -1;
    (*items)[*count].size = archive_entry_size_is_set(entry) && archive_entry_size(entry) > 0
        ? (uint64_t)archive_entry_size(entry) : 0;
    (*items)[*count].directory = archive_entry_filetype(entry) == AE_IFDIR;
    ++*count;
    return 0;
}

static int read_archive(const char *path, struct archive_item **items_out,
                        size_t *count_out, char *error, size_t error_size)
{
    struct archive *archive;
    struct archive_entry *entry;
    struct archive_item *items = NULL;
    size_t count = 0, capacity = 0;
    int status;

    archive = archive_read_new();
    if (archive == NULL) {
        errno = ENOMEM;
        return -1;
    }
    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);
    if (archive_read_open_filename(archive, path, 10240) != ARCHIVE_OK) {
        (void)snprintf(error, error_size, "%s",
                       archive_error_string(archive) != NULL ? archive_error_string(archive) : "cannot open archive");
        archive_read_free(archive);
        errno = EINVAL;
        return -1;
    }
    while ((status = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        if (append_item(&items, &count, &capacity, entry) != 0) {
            int saved_errno = errno;
            free_items(items, count);
            archive_read_free(archive);
            errno = saved_errno;
            return -1;
        }
        if (archive_read_data_skip(archive) != ARCHIVE_OK) {
            (void)snprintf(error, error_size, "%s",
                           archive_error_string(archive) != NULL ? archive_error_string(archive) : "archive read error");
            free_items(items, count);
            archive_read_free(archive);
            errno = EINVAL;
            return -1;
        }
    }
    if (status != ARCHIVE_EOF) {
        (void)snprintf(error, error_size, "%s",
                       archive_error_string(archive) != NULL ? archive_error_string(archive) : "invalid archive");
        free_items(items, count);
        archive_read_free(archive);
        errno = EINVAL;
        return -1;
    }
    archive_read_free(archive);
    *items_out = items;
    *count_out = count;
    return 0;
}

void archive_preview_open(const char *path)
{
    struct archive_item *items = NULL;
    size_t count = 0;
    int selected = 0, scroll = 0, key;
    char error[512] = "";
    WINDOW *window;
    int rows, columns, height, width, body;

    if (path == NULL || !archive_preview_is_supported(path)) {
        errno = EINVAL;
        return;
    }
    if (read_archive(path, &items, &count, error, sizeof(error)) != 0) {
        int saved_errno = errno;
        char message[1024];
        (void)snprintf(message, sizeof(message), "Cannot preview archive:\n%s\n\n%s",
                       path, error[0] != '\0' ? error : strerror(saved_errno));
        dialog_text_view("Archive preview failed", message);
        return;
    }

    getmaxyx(stdscr, rows, columns);
    height = rows > 5 ? rows - 2 : rows;
    width = columns > 10 ? columns - 2 : columns;
    if (height < 4 || width < 10) {
        free_items(items, count);
        return;
    }
    window = newwin(height, width, (rows - height) / 2, (columns - width) / 2);
    if (window == NULL) {
        free_items(items, count);
        return;
    }
    (void)keypad(window, TRUE);
    body = height - 3;
    if (body < 1) body = 1;

    for (;;) {
        size_t i;
        (void)werase(window);
        (void)box(window, 0, 0);
        (void)wattrset(window, A_BOLD);
        (void)mvwaddnstr(window, 0, 2, " Archive Preview ", width - 4);
        (void)wattrset(window, A_NORMAL);
        (void)mvwaddnstr(window, 1, 2, path, width - 4);
        for (i = (size_t)scroll; i < count && (int)i < scroll + body; ++i) {
            char size_text[32];
            int y = 2 + (int)i - scroll;
            (void)snprintf(size_text, sizeof(size_text), "%llu",
                           (unsigned long long)items[i].size);
            (void)wattrset(window, (int)i == selected ? A_REVERSE | A_BOLD : A_NORMAL);
            (void)mvwaddnstr(window, y, 2, items[i].directory ? "[DIR] " : "      ", 6);
            (void)mvwaddnstr(window, y, 8, items[i].name, width - 22);
            if (width > 24)
                (void)mvwaddnstr(window, y, width - 13, size_text, 10);
        }
        (void)wattrset(window, A_REVERSE);
        (void)mvwhline(window, height - 1, 1, ' ', width - 2);
        (void)mvwprintw(window, height - 1, 2,
                        " %lu entries | Up/Down PgUp/PgDn | Esc: Back ", (unsigned long)count);
        (void)wattrset(window, A_NORMAL);
        (void)wrefresh(window);

        key = wgetch(window);
        if (key == 27 || key == 'q' || key == 'Q')
            break;
        if (key == KEY_UP && selected > 0) --selected;
        else if (key == KEY_DOWN && (size_t)(selected + 1) < count) ++selected;
        else if (key == KEY_NPAGE && (size_t)(selected + body) < count) selected += body;
        else if (key == KEY_PPAGE) { selected -= body; if (selected < 0) selected = 0; }
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + body) scroll = selected - body + 1;
    }
    (void)delwin(window);
    (void)touchwin(stdscr);
    (void)refresh();
    free_items(items, count);
}
