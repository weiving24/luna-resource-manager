#include "open.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dialogs.h"
#include "archive_preview.h"
#include "fileops.h"
#include "menu.h"
#include "process.h"
#include "viewer.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int is_executable_file(const char *path)
{
struct stat status;

return path != NULL &&
       stat(path, &status) == 0 &&
       S_ISREG(status.st_mode) &&
       access(path, X_OK) == 0;

}

static int is_text_file(const char *path)
{
FILE *stream;
unsigned char buffer[4096];
size_t bytes;
size_t i;

if (path == NULL)
    return 0;

stream = fopen(path, "rb");
if (stream == NULL)
    return 0;

bytes = fread(buffer, 1, sizeof(buffer), stream);

if (ferror(stream)) {
    (void)fclose(stream);
    return 0;
}

(void)fclose(stream);

for (i = 0; i < bytes; ++i) {
    if (buffer[i] == '\0')
        return 0;
}

return 1;

}

static void open_system_default(const char *path)
{
const char *configured = getenv("LUNA_RESOURCE_MANAGER_OPEN");
int result;

if (configured != NULL && configured[0] != '\0') {
    result = process_run_external(configured, path);
} else {

#ifdef APPLE
result = process_run_external("open", path);
#else
result = process_run_external("xdg-open", path);
#endif
}

if (result != 0) {
    dialog_message(
        "Open failed",
        "No system file opener succeeded. "
        "Set LUNA_RESOURCE_MANAGER_OPEN to an opener program.");
}

}

static void run_file(const char *path)
{
char *output = NULL;
size_t length = 0;
int exit_code = -1;
char title[PATH_MAX + 32];

if (process_run_file_capture(path, &output, &length, &exit_code) != 0) {
    char message[PATH_MAX + 128];

    (void)snprintf(message, sizeof(message),
                   "Could not run:\n%s\n\n%s",
                   path, strerror(errno));

    dialog_text_view("Execution failed", message);
    return;
}

(void)snprintf(title, sizeof(title), " Output: %.4000s ", path);
viewer_output(title, output, length, exit_code);
free(output);

}

static void secondary_open_menu(const char *path)
{
MenuOption options[5];
int count = 0;
int selected;
int is_executable;

if (path == NULL)
    return;

if (archive_preview_is_supported(path)) {
    archive_preview_open(path);
    return;
}

is_executable = is_executable_file(path);

options[count++] = (MenuOption){ "View (built-in)", "view" };
options[count++] = (MenuOption){ "Editor (vi)", "vi" };
options[count++] = (MenuOption){ "Pager (less)", "less" };
options[count++] = (MenuOption){ "System default", "default" };

if (is_executable)
    options[count++] = (MenuOption){
        "Run and capture output", "run"
    };

selected = menu_select("Open With", path, options, count);

if (selected < 0)
    return;

if (strcmp(options[selected].value, "view") == 0) {
    if (is_text_file(path))
        viewer_text_file(path);
    else
        open_system_default(path);
} else if (strcmp(options[selected].value, "vi") == 0) {
    int result = process_run_external("vi", path);

    if (result < 0 || result == 127)
        dialog_message("Editor failed", "Could not launch vi.");
} else if (strcmp(options[selected].value, "less") == 0) {
    int result = process_run_external("less", path);

    if (result < 0 || result == 127)
        dialog_message("Pager failed", "Could not launch less.");
} else if (strcmp(options[selected].value, "default") == 0) {
    open_system_default(path);
} else if (strcmp(options[selected].value, "run") == 0) {
    run_file(path);
}

}

void action_menu_interactive(const char *path,
const char *start_dir,
AppState *state)
{
static const MenuOption options[] = {
{ "Open", "open" },
{ "Copy", "copy" },
{ "Move", "move" },
{ "Rename", "rename" },
{ "Delete", "delete" }
};

int selected;

if (path == NULL)
    return;

selected = menu_select(
    "File Actions",
    path,
    options,
    (int)(sizeof(options) / sizeof(options[0])));

if (selected < 0)
    return;

if (strcmp(options[selected].value, "open") == 0) {
    struct stat status;

    if (stat(path, &status) == 0 && S_ISDIR(status.st_mode)) {
        if (state != NULL && state_set_cwd(state, path) == 0) {
            (void)snprintf(state->status,
                           sizeof(state->status),
                           "Entered directory");
        } else if (state != NULL) {
            (void)snprintf(state->status,
                           sizeof(state->status),
                           "Cannot enter: %s",
                           strerror(errno));
        }
    } else if (archive_preview_is_supported(path)) {
        archive_preview_open(path);
    } else {
        secondary_open_menu(path);
    }
} else if (strcmp(options[selected].value, "copy") == 0) {
    fileops_copy(state, path, start_dir);
} else if (strcmp(options[selected].value, "move") == 0) {
    fileops_move(state, path, start_dir);
} else if (strcmp(options[selected].value, "rename") == 0) {
    fileops_rename(state, path);
} else if (strcmp(options[selected].value, "delete") == 0) {
    fileops_delete(state, path);
}

}

void open_file_interactive(const char *path)
{
if (path == NULL)
return;

secondary_open_menu(path);

}
