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
#include <strings.h>

#define ARCHIVE_ITEM_LIMIT 1000000U
#define ARCHIVE_FILE_LIMIT (32U * 1024U * 1024U)
#define ARCHIVE_INDEX_INITIAL 64U
#define ARCHIVE_PATH_LIMIT 4096U

struct archive_item {
    char *path;
    uint64_t size;
    int directory;
    int regular;
};

struct visible_item {
    size_t index;
    const char *name;
};

struct archive_index {
    struct archive_item *items;
    size_t count;
    size_t capacity;
};

static int has_suffix(const char *path, const char *suffix)
{
    size_t a;
    size_t b;

    if (path == NULL || suffix == NULL)
        return 0;

    a = strlen(path);
    b = strlen(suffix);

    return a >= b && strcasecmp(path + a - b, suffix) == 0;
}

int archive_preview_is_supported(const char *path)
{
    static const char *const suffixes[] = {
        ".zip", ".tar", ".tar.gz", ".tgz",
        ".tar.bz2", ".tbz2", ".tar.xz", ".txz",
        ".tar.zst", ".tzst", ".tar.lz", ".tlz",
        ".7z", ".rar", ".cab", ".iso", ".ar", ".cpio"
    };
    size_t i;

    if (path == NULL)
        return 0;

    for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
        if (has_suffix(path, suffixes[i]))
            return 1;
    }

    return 0;
}

static void free_index(struct archive_index *index)
{
    size_t i;

    for (i = 0; i < index->count; ++i)
        free(index->items[i].path);

    free(index->items);
    memset(index, 0, sizeof(*index));
}

static char *normalize_path(const char *input)
{
    size_t length;
    size_t read_pos = 0;
    size_t write_pos = 0;
    char *output;

    if (input == NULL)
        return NULL;

    length = strlen(input);

    if (length == 0 || length >= ARCHIVE_PATH_LIMIT)
        return NULL;

    if (input[0] == '/' || input[0] == '\\')
        return NULL;

    if (length >= 2 &&
        ((input[0] >= 'A' && input[0] <= 'Z') ||
         (input[0] >= 'a' && input[0] <= 'z')) &&
        input[1] == ':')
        return NULL;

    output = malloc(length + 1);
    if (output == NULL)
        return NULL;

    while (read_pos < length) {
        size_t start;
        size_t part_length;

        while (read_pos < length &&
               (input[read_pos] == '/' || input[read_pos] == '\\'))
            ++read_pos;

        start = read_pos;

        while (read_pos < length &&
               input[read_pos] != '/' && input[read_pos] != '\\')
            ++read_pos;

        part_length = read_pos - start;

        if (part_length == 0)
            continue;

        if (part_length == 1 && input[start] == '.')
            continue;

        if (part_length == 2 &&
            input[start] == '.' && input[start + 1] == '.') {
            free(output);
            return NULL;
        }

        if (write_pos != 0)
            output[write_pos++] = '/';

        memcpy(output + write_pos, input + start, part_length);
        write_pos += part_length;
    }

    if (write_pos == 0) {
        free(output);
        return NULL;
    }

    output[write_pos] = '\0';
    return output;
}

static int index_find(const struct archive_index *index, const char *path)
{
    size_t i;

    for (i = 0; i < index->count; ++i) {
        if (strcmp(index->items[i].path, path) == 0)
            return (int)i;
    }

    return -1;
}

static int index_add(struct archive_index *index, const char *path,
                     uint64_t size, int directory, int regular)
{
    struct archive_item *grown;
    char *copy;
    size_t next;
    int found;

    found = index_find(index, path);

    if (found >= 0) {
        struct archive_item *item = &index->items[found];

        if (directory)
            item->directory = 1;

        if (regular) {
            item->regular = 1;
            item->size = size;
        }

        return 0;
    }

    if (index->count >= ARCHIVE_ITEM_LIMIT) {
        errno = EFBIG;
        return -1;
    }

    if (index->count == index->capacity) {
        next = index->capacity == 0
            ? ARCHIVE_INDEX_INITIAL
            : index->capacity * 2;

        if (next < index->capacity || next > ARCHIVE_ITEM_LIMIT)
            next = ARCHIVE_ITEM_LIMIT;

        grown = realloc(index->items, next * sizeof(*grown));
        if (grown == NULL)
            return -1;

        index->items = grown;
        index->capacity = next;
    }

    copy = strdup(path);
    if (copy == NULL)
        return -1;

    index->items[index->count].path = copy;
    index->items[index->count].size = size;
    index->items[index->count].directory = directory;
    index->items[index->count].regular = regular;
    ++index->count;

    return 0;
}

static int add_path_and_parents(struct archive_index *index,
                                const char *path, uint64_t size,
                                int directory, int regular)
{
    char *copy;
    char *slash;

    copy = strdup(path);
    if (copy == NULL)
        return -1;

    slash = copy;

    while ((slash = strchr(slash, '/')) != NULL) {
        char saved = *slash;

        *slash = '\0';

        if (index_add(index, copy, 0, 1, 0) != 0) {
            free(copy);
            return -1;
        }

        *slash = saved;
        ++slash;
    }

    {
        int result = index_add(index, path, size, directory, regular);
        free(copy);
        return result;
    }
}

static struct archive *open_archive_reader(const char *path,
                                           char *error,
                                           size_t error_size)
{
    struct archive *archive = archive_read_new();

    if (archive == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);

    if (archive_read_open_filename(archive, path, 10240) != ARCHIVE_OK) {
        const char *message = archive_error_string(archive);

        if (error != NULL && error_size != 0)
            snprintf(error, error_size, "%s",
                     message != NULL ? message : "Cannot open archive.");

        archive_read_free(archive);
        errno = EINVAL;
        return NULL;
    }

    return archive;
}

static int build_index(const char *path, struct archive_index *index,
                       char *error, size_t error_size)
{
    struct archive *archive;
    struct archive_entry *entry;
    int status;

    archive = open_archive_reader(path, error, error_size);
    if (archive == NULL)
        return -1;

    while ((status = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        const char *raw_path = archive_entry_pathname(entry);
        char *normalized;
        mode_t type;
        uint64_t size = 0;
        int directory;
        int regular;

        if (raw_path == NULL) {
            archive_read_data_skip(archive);
            continue;
        }

        normalized = normalize_path(raw_path);

        if (normalized == NULL) {
            archive_read_data_skip(archive);
            continue;
        }

        type = archive_entry_filetype(entry);
        directory = type == AE_IFDIR;
        regular = type == AE_IFREG;

        if (regular && archive_entry_size_is_set(entry) &&
            archive_entry_size(entry) > 0)
            size = (uint64_t)archive_entry_size(entry);

        if ((directory || regular) &&
            add_path_and_parents(index, normalized, size,
                                 directory, regular) != 0) {
            int saved_errno = errno;

            free(normalized);
            archive_read_free(archive);
            errno = saved_errno;
            return -1;
        }

        free(normalized);

        if (archive_read_data_skip(archive) != ARCHIVE_OK) {
            const char *message = archive_error_string(archive);

            if (error != NULL && error_size != 0)
                snprintf(error, error_size, "%s",
                         message != NULL ? message : "Archive read error.");

            archive_read_free(archive);
            errno = EINVAL;
            return -1;
        }
    }

    if (status != ARCHIVE_EOF) {
        const char *message = archive_error_string(archive);

        if (error != NULL && error_size != 0)
            snprintf(error, error_size, "%s",
                     message != NULL ? message : "Invalid archive.");

        archive_read_free(archive);
        errno = EINVAL;
        return -1;
    }

    archive_read_free(archive);
    return 0;
}

static int is_immediate_child(const char *current_dir, const char *path,
                              const char **name_out)
{
    size_t parent_length;
    const char *rest;

    parent_length = strlen(current_dir);

    if (parent_length == 0) {
        rest = path;
    } else {
        if (strncmp(path, current_dir, parent_length) != 0)
            return 0;

        if (path[parent_length] != '/')
            return 0;

        rest = path + parent_length + 1;
    }

    if (*rest == '\0' || strchr(rest, '/') != NULL)
        return 0;

    *name_out = rest;
    return 1;
}

static int build_visible(const struct archive_index *index,
                         const char *current_dir,
                         struct visible_item **visible_out,
                         size_t *visible_count_out)
{
    struct visible_item *visible = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t i;

    for (i = 0; i < index->count; ++i) {
        const char *name;
        size_t j;
        int duplicate = 0;

        if (!is_immediate_child(current_dir, index->items[i].path, &name))
            continue;

        for (j = 0; j < count; ++j) {
            if (strcmp(visible[j].name, name) == 0) {
                duplicate = 1;
                break;
            }
        }

        if (duplicate)
            continue;

        if (count == capacity) {
            size_t next = capacity == 0 ? 32 : capacity * 2;
            struct visible_item *grown;

            if (next < capacity || next > ARCHIVE_ITEM_LIMIT) {
                free(visible);
                errno = EFBIG;
                return -1;
            }

            grown = realloc(visible, next * sizeof(*grown));
            if (grown == NULL) {
                free(visible);
                return -1;
            }

            visible = grown;
            capacity = next;
        }

        visible[count].index = i;
        visible[count].name = name;
        ++count;
    }

    *visible_out = visible;
    *visible_count_out = count;
    return 0;
}

static char *join_path(const char *parent, const char *name)
{
    size_t a = strlen(parent);
    size_t b = strlen(name);
    char *result;

    if (a + b + 2 > ARCHIVE_PATH_LIMIT) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    result = malloc(a + b + 2);
    if (result == NULL)
        return NULL;

    if (a == 0)
        snprintf(result, a + b + 2, "%s", name);
    else
        snprintf(result, a + b + 2, "%s/%s", parent, name);

    return result;
}

static void draw_text_buffer(const char *title,
                             const unsigned char *data, size_t length)
{
    WINDOW *window;
    int rows;
    int columns;
    int height;
    int width;
    int body;
    int scroll = 0;
    int key;
    int total_lines = 1;
    size_t i;

    getmaxyx(stdscr, rows, columns);

    height = rows * 3 / 4;
    width = columns * 4 / 5;

    if (height < 4)
        height = rows;

    if (width < 10)
        width = columns;

    if (height > rows)
        height = rows;

    if (width > columns)
        width = columns;

    if (height < 4 || width < 10)
        return;

    for (i = 0; i < length; ++i) {
        if (data[i] == '\n')
            ++total_lines;
    }

    window = newwin(height, width,
                    (rows - height) / 2, (columns - width) / 2);

    if (window == NULL)
        return;

    keypad(window, TRUE);

    body = height - 2;
    if (body < 1)
        body = 1;

    for (;;) {
        size_t pos = 0;
        int line = 0;
        int visible_line = 0;

        werase(window);
        box(window, 0, 0);
        wattrset(window, A_BOLD);
        mvwaddnstr(window, 0, 2, title, width - 4);
        wattrset(window, A_NORMAL);

        while (pos < length || (length == 0 && line == 0)) {
            size_t end = pos;
            size_t shown;
            size_t j;

            while (end < length && data[end] != '\n')
                ++end;

            shown = end - pos;
            if (shown > (size_t)(width - 2))
                shown = (size_t)(width - 2);

            if (line >= scroll && visible_line < body) {
                char *line_buffer = malloc(shown + 1);

                if (line_buffer != NULL) {
                    for (j = 0; j < shown; ++j) {
                        unsigned char c = data[pos + j];

                        line_buffer[j] =
                            (c == '\t') ? ' ' :
                            (c < 32 || c == 127) ? '.' : (char)c;
                    }

                    line_buffer[shown] = '\0';
                    mvwaddnstr(window, 1 + visible_line, 1,
                               line_buffer, (int)shown);
                    free(line_buffer);
                }

                ++visible_line;
            }

            if (end < length)
                pos = end + 1;
            else
                pos = length;

            ++line;

            if (pos >= length)
                break;
        }

        wattrset(window, A_REVERSE);
        mvwhline(window, height - 1, 1, ' ', width - 2);
        mvwprintw(window, height - 1, 2,
                  " Esc/q: Close  Up/Down: Scroll  %d lines ",
                  total_lines);
        wattrset(window, A_NORMAL);

        wnoutrefresh(window);
        doupdate();

        key = wgetch(window);

        if (key == 27 || key == 'q' || key == 'Q')
            break;

        if (key == KEY_UP && scroll > 0)
            --scroll;
        else if (key == KEY_DOWN && scroll < total_lines - 1)
            ++scroll;
        else if (key == KEY_NPAGE) {
            scroll += body;
            if (scroll > total_lines - 1)
                scroll = total_lines - 1;
        } else if (key == KEY_PPAGE) {
            scroll -= body;
            if (scroll < 0)
                scroll = 0;
        }
    }

    delwin(window);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
}

static int buffer_is_binary(const unsigned char *data, size_t length)
{
    size_t i;
    size_t limit = length < 8192 ? length : 8192;

    for (i = 0; i < limit; ++i) {
        if (data[i] == '\0')
            return 1;
    }

    return 0;
}

static void show_binary_buffer(const char *title,
                               const unsigned char *data, size_t length)
{
    size_t offset = 0;
    size_t capacity;
    size_t used = 0;
    char *output;

    if (length > (SIZE_MAX - 1024) / 6) {
        dialog_text_view("Binary preview", "File is too large to format.");
        return;
    }

    capacity = length * 6 + 1024;
    output = malloc(capacity);

    if (output == NULL) {
        dialog_text_view("Binary preview", "Not enough memory.");
        return;
    }

    while (offset < length && used + 128 < capacity) {
        size_t i;
        int written;

        written = snprintf(output + used, capacity - used,
                           "%08llx  ", (unsigned long long)offset);
        if (written < 0)
            break;

        used += (size_t)written;

        for (i = 0; i < 16 && offset + i < length; ++i) {
            written = snprintf(output + used, capacity - used,
                               "%02x ", data[offset + i]);
            if (written < 0)
                break;
            used += (size_t)written;
        }

        if (used + 64 >= capacity)
            break;

        for (; i < 16; ++i) {
            output[used++] = ' ';
            output[used++] = ' ';
            output[used++] = ' ';
        }

        output[used++] = ' ';

        for (i = 0; i < 16 && offset + i < length; ++i) {
            unsigned char c = data[offset + i];
            output[used++] = (c >= 32 && c < 127) ? (char)c : '.';
        }

        output[used++] = '\n';
        offset += 16;
    }

    output[used] = '\0';
    draw_text_buffer(title, (const unsigned char *)output, used);
    free(output);
}

static void preview_buffer(const char *name,
                           const unsigned char *data, size_t length)
{
    char title[512];

    snprintf(title, sizeof(title), " Preview: %.470s ", name);

    if (buffer_is_binary(data, length))
        show_binary_buffer(title, data, length);
    else
        draw_text_buffer(title, data, length);
}

static int read_selected_entry(const char *archive_path,
                               const char *wanted_path,
                               unsigned char **data_out,
                               size_t *length_out,
                               char *error, size_t error_size)
{
    struct archive *archive;
    struct archive_entry *entry;
    unsigned char *buffer = NULL;
    size_t used = 0;
    size_t capacity = 0;
    int status;
    int found = 0;

    archive = open_archive_reader(archive_path, error, error_size);
    if (archive == NULL)
        return -1;

    while ((status = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
        const char *raw_path = archive_entry_pathname(entry);
        char *normalized;

        if (raw_path == NULL) {
            archive_read_data_skip(archive);
            continue;
        }

        normalized = normalize_path(raw_path);

        if (normalized == NULL) {
            archive_read_data_skip(archive);
            continue;
        }

        if (strcmp(normalized, wanted_path) != 0) {
            free(normalized);

            if (archive_read_data_skip(archive) != ARCHIVE_OK) {
                snprintf(error, error_size, "%s",
                         archive_error_string(archive) != NULL
                            ? archive_error_string(archive)
                            : "Archive read error.");
                goto fail;
            }

            continue;
        }

        free(normalized);

        if (archive_entry_filetype(entry) != AE_IFREG) {
            snprintf(error, error_size, "%s", "Not a regular file.");
            goto fail;
        }

        if (archive_entry_size_is_set(entry) &&
            archive_entry_size(entry) > (la_int64_t)ARCHIVE_FILE_LIMIT) {
            snprintf(error, error_size,
                     "File exceeds the 32 MiB in-memory preview limit.");
            goto fail;
        }

        found = 1;

        for (;;) {
            la_ssize_t got;

            if (used == capacity) {
                size_t next;
                unsigned char *grown;

                if (capacity >= ARCHIVE_FILE_LIMIT) {
                    unsigned char extra;

                    got = archive_read_data(archive, &extra, 1);

                    if (got > 0) {
                        snprintf(error, error_size,
                                 "File exceeds the 32 MiB in-memory preview limit.");
                        goto fail;
                    }

                    if (got < 0) {
                        snprintf(error, error_size, "%s",
                                 archive_error_string(archive) != NULL
                                    ? archive_error_string(archive)
                                    : "Archive read error.");
                        goto fail;
                    }

                    break;
                }

                next = capacity == 0 ? 8192 : capacity * 2;

                if (next > ARCHIVE_FILE_LIMIT)
                    next = ARCHIVE_FILE_LIMIT;

                grown = realloc(buffer, next);

                if (grown == NULL) {
                    snprintf(error, error_size,
                             "Not enough memory to preview this file.");
                    goto fail;
                }

                buffer = grown;
                capacity = next;
            }

            got = archive_read_data(archive, buffer + used,
                                    capacity - used);

            if (got < 0) {
                snprintf(error, error_size, "%s",
                         archive_error_string(archive) != NULL
                            ? archive_error_string(archive)
                            : "Archive read error.");
                goto fail;
            }

            if (got == 0)
                break;

            used += (size_t)got;
        }

        break;
    }

    if (status != ARCHIVE_OK && status != ARCHIVE_EOF && !found) {
        snprintf(error, error_size, "%s",
                 archive_error_string(archive) != NULL
                    ? archive_error_string(archive)
                    : "Archive read error.");
        goto fail;
    }

    archive_read_free(archive);

    if (!found) {
        free(buffer);
        snprintf(error, error_size, "%s", "Selected entry not found.");
        errno = ENOENT;
        return -1;
    }

    if (buffer == NULL) {
        buffer = malloc(1);

        if (buffer == NULL)
            return -1;
    }

    *data_out = buffer;
    *length_out = used;
    return 0;

fail:
    free(buffer);
    archive_read_free(archive);
    errno = EINVAL;
    return -1;
}

static void show_entry_error(const char *name, const char *error)
{
    char message[1024];

    snprintf(message, sizeof(message),
             "Cannot preview entry:\n%.400s\n\n%.500s",
             name != NULL ? name : "(unknown)",
             error != NULL && error[0] != '\0'
                ? error : strerror(errno));

    dialog_text_view("Archive entry error", message);
}

static void draw_archive_list(WINDOW *window, const char *path,
                              const char *current_dir,
                              const struct archive_index *index,
                              const struct visible_item *visible,
                              size_t visible_count,
                              int selected, int scroll,
                              int body, int height, int width)
{
    size_t i;
    char location[ARCHIVE_PATH_LIMIT + 16];

    werase(window);
    box(window, 0, 0);

    wattrset(window, A_BOLD);
    mvwaddnstr(window, 0, 2, " Archive Preview ", width - 4);
    wattrset(window, A_NORMAL);

    mvwaddnstr(window, 1, 2, path, width - 4);

    if (current_dir[0] == '\0')
        snprintf(location, sizeof(location), "/");
    else
        snprintf(location, sizeof(location), "/%s", current_dir);

    mvwaddnstr(window, 2, 2, location, width - 4);

    for (i = (size_t)scroll;
         i < visible_count && (int)i < scroll + body; ++i) {
        const struct archive_item *item =
            &index->items[visible[i].index];
        char size_text[32];
        int y = 3 + (int)i - scroll;
        int name_width = width - 24;

        if (name_width < 1)
            name_width = 1;

        if (item->directory)
            snprintf(size_text, sizeof(size_text), "<DIR>");
        else
            snprintf(size_text, sizeof(size_text), "%llu",
                     (unsigned long long)item->size);

        wattrset(window, (int)i == selected
                            ? A_REVERSE | A_BOLD : A_NORMAL);

        mvwaddnstr(window, y, 2,
                   item->directory ? "[DIR] " : "      ", 6);
        mvwaddnstr(window, y, 8, visible[i].name, name_width);

        if (width > 24)
            mvwaddnstr(window, y, width - 14, size_text, 12);
    }

    wattrset(window, A_REVERSE);
    mvwhline(window, height - 1, 1, ' ', width - 2);
    mvwprintw(window, height - 1, 2,
              " %lu entries | Enter: Open | Backspace: Back | Esc: Exit ",
              (unsigned long)visible_count);
    wattrset(window, A_NORMAL);

    wnoutrefresh(window);
    doupdate();
}

void archive_preview_open(const char *path)
{
    struct archive_index index = {0};
    char current_dir[ARCHIVE_PATH_LIMIT] = "";
    char error[512] = "";
    int selected = 0;
    int scroll = 0;
    WINDOW *window = NULL;
    int window_height = 0;
    int window_width = 0;
    int body = 0;

    if (path == NULL || !archive_preview_is_supported(path)) {
        errno = EINVAL;
        return;
    }

    if (build_index(path, &index, error, sizeof(error)) != 0) {
        char message[1024];

        snprintf(message, sizeof(message),
                 "Cannot open archive:\n%.400s\n\n%.500s",
                 path, error[0] != '\0' ? error : strerror(errno));

        dialog_text_view("Archive preview failed", message);
        free_index(&index);
        return;
    }

    for (;;) {
        struct visible_item *visible = NULL;
        size_t visible_count = 0;
        int rows;
        int columns;
        int key;
        int recreate_window = 0;

        if (build_visible(&index, current_dir,
                          &visible, &visible_count) != 0) {
            dialog_text_view("Archive preview",
                             "Not enough memory to build directory view.");
            free(visible);
            break;
        }

        if (selected < 0)
            selected = 0;

        if (visible_count == 0) {
            selected = 0;
            scroll = 0;
        } else if ((size_t)selected >= visible_count) {
            selected = (int)visible_count - 1;
        }

        getmaxyx(stdscr, rows, columns);

        if (window == NULL ||
            window_height != (rows > 5 ? rows - 2 : rows) ||
            window_width != (columns > 10 ? columns - 2 : columns)) {
            if (window != NULL)
                delwin(window);

            window_height = rows > 5 ? rows - 2 : rows;
            window_width = columns > 10 ? columns - 2 : columns;

            if (window_height < 4 || window_width < 10) {
                free(visible);
                break;
            }

            window = newwin(window_height, window_width,
                            (rows - window_height) / 2,
                            (columns - window_width) / 2);

            if (window == NULL) {
                free(visible);
                break;
            }

            keypad(window, TRUE);
            body = window_height - 4;

            if (body < 1)
                body = 1;

            recreate_window = 1;
        }

        if (selected < scroll)
            scroll = selected;

        if (selected >= scroll + body)
            scroll = selected - body + 1;

        if (scroll < 0)
            scroll = 0;

        if (recreate_window)
            werase(window);

        draw_archive_list(window, path, current_dir, &index,
                          visible, visible_count, selected, scroll,
                          body, window_height, window_width);

        key = wgetch(window);

        if (key == 27 || key == 'q' || key == 'Q') {
            free(visible);
            break;
        }

        if (key == KEY_UP) {
            if (selected > 0)
                --selected;
        } else if (key == KEY_DOWN) {
            if ((size_t)(selected + 1) < visible_count)
                ++selected;
        } else if (key == KEY_NPAGE) {
            selected += body;

            if (selected >= (int)visible_count)
                selected = visible_count > 0
                    ? (int)visible_count - 1 : 0;
        } else if (key == KEY_PPAGE) {
            selected -= body;

            if (selected < 0)
                selected = 0;
        } else if (key == KEY_BACKSPACE || key == 127 ||
                   key == 8 || key == KEY_LEFT) {
            if (current_dir[0] == '\0') {
                free(visible);
                break;
            }

            {
                char *slash = strrchr(current_dir, '/');

                if (slash != NULL)
                    *slash = '\0';
                else
                    current_dir[0] = '\0';
            }

            selected = 0;
            scroll = 0;
        } else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            if (visible_count != 0 && selected >= 0 &&
                (size_t)selected < visible_count) {
                const struct visible_item *chosen = &visible[selected];
                const struct archive_item *item =
                    &index.items[chosen->index];

                if (item->directory) {
                    char *next = join_path(current_dir, chosen->name);

                    if (next != NULL) {
                        snprintf(current_dir, sizeof(current_dir), "%s", next);
                        free(next);
                        selected = 0;
                        scroll = 0;
                    } else {
                        dialog_text_view("Archive preview",
                                         "Directory path is too long.");
                    }
                } else if (item->regular) {
                    unsigned char *data = NULL;
                    size_t length = 0;
                    char *entry_path = join_path(current_dir, chosen->name);

                    if (entry_path == NULL) {
                        dialog_text_view("Archive preview",
                                         "Entry path is too long.");
                    } else {
                        error[0] = '\0';

                        if (read_selected_entry(path, entry_path,
                                &data, &length,
                                error, sizeof(error)) == 0) {
                            preview_buffer(entry_path, data, length);
                            free(data);
                        } else {
                            show_entry_error(entry_path, error);
                        }

                        free(entry_path);
                    }

                    touchwin(stdscr);
                    wnoutrefresh(stdscr);
                    clearok(window, TRUE);
                }
            }
        }

        free(visible);
    }

    if (window != NULL)
        delwin(window);

    free_index(&index);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
}