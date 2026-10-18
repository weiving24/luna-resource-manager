#include "open.h"
#include "dialogs.h"
#include <stdio.h>

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "Luna Resource Manager"
#endif
#ifndef LUNA_VERSION
#define LUNA_VERSION "unknown"
#endif

void show_about_dialog(void)
{
    static const char text[] =
        "About\n\n"
        "A modular terminal file manager written in C.\n\n"
        "Core file operations use POSIX filesystem calls.\n"
        "The terminal UI uses the curses library.\n"
        "Archive preview is powered by libarchive.\n\n"
        "Modules:\n"
        "  core/fs.c       Directory listing and preview\n"
        "  core/fs_ops.c   Copy, move and recursive removal\n"
        "  uidl/           UI layout and theme model\n"
        "  prt/            Terminal renderer\n"
        "  app/            State, dialogs, config and actions\n\n"
        "License:\n"
        "  GNU GPLv3 (https://www.gnu.org/licenses/gpl-3.0.html)\n"
        "  THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY.\n\n"
        "Author: weiving24\n"
        "Press Esc or q to close.";
    char full_text[2048];
    snprintf(full_text, sizeof(full_text), "%s v%s\n%s", PROGRAM_NAME, LUNA_VERSION, text);
    dialog_text_view("About", full_text);
}

void show_help_dialog(void)
{
    static const char text[] =
        "USAGE\n\n"
        "  %s [options] [directory]\n\n"
        "NAVIGATION\n\n"
        "  Up / Down       Move selection\n"
        "  PageUp / PgDn   Move by page\n"
        "  Home / End      First / last item\n"
        "  Enter / Right   Enter directory or open file\n"
        "  Left            Go to parent directory\n"
        "  F2              Actions for selected item\n"
        "  Right-click     Preview archive or open item actions\n"
        "  n               Create directory\n"
        "  q               Quit\n\n"
        "FILE ACTIONS\n\n"
        "  Open, Copy, Move, Rename, Delete\n"
        "  Open supports a built-in archive preview for ZIP, TAR, and 7z.\n"
        "  Copy and Move let you choose a destination directory.\n"
        "  Delete always asks for confirmation.\n\n"
        "OTHER KEYS\n\n"
        "  a               About\n"
        "  ? / F1          Help\n\n"
        "Press Esc or q to close.";
    char full_text[2048];
    snprintf(full_text, sizeof(full_text), text, "luna-resource-manager");
    dialog_text_view("Help", full_text);
}