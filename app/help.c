#include "open.h"

#include "dialogs.h"

void show_about_dialog(void)
{
    static const char text[] =
        "Luna Resource Manager\n\n"
        "A modular terminal file manager written in C.\n\n"
        "Core file operations use POSIX filesystem calls.\n"
        "The terminal UI uses the curses library.\n\n"
        "Modules:\n"
        "  core/fs.c       Directory listing and preview\n"
        "  core/fs_ops.c   Copy, move and recursive removal\n"
        "  uidl/           UI layout and theme model\n"
        "  prt/            Terminal renderer\n"
        "  app/            State, dialogs, config and actions\n\n"
        "Press Esc or q to close.";
    dialog_text_view("About", text);
}

void show_help_dialog(void)
{
    static const char text[] =
        "USAGE\n\n"
        "  luna-resource-manager [options] [directory]\n\n"
        "NAVIGATION\n\n"
        "  Up / Down       Move selection\n"
        "  PageUp / PgDn   Move by page\n"
        "  Home / End      First / last item\n"
        "  Enter / Right   Enter directory or open file\n"
        "  Left            Go to parent directory\n"
        "  F2              Actions for selected item\n"
        "  Right-click      Preview an archive or open item actions\n"
        "  n               Create directory\n"
        "  q               Quit\n\n"
        "FILE ACTIONS\n\n"
        "  Open, Copy, Move, Rename, Delete\n"
        "  Open supports a built-in archive preview for supported formats.\n"
        "  Copy and Move let you choose a destination directory.\n"
        "  Delete always asks for confirmation.\n\n"
        "OTHER KEYS\n\n"
        "  a               About\n"
        "  ? / F1          Help\n\n"
        "Press Esc or q to close.";
    dialog_text_view("Help", text);
}
