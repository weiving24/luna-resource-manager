[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md) | [日本語](README.ja.md)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![C99](https://img.shields.io/badge/C-C99-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey.svg)
![UI](https://img.shields.io/badge/UI-curses-green.svg)
![Archive](https://img.shields.io/badge/Archive-libarchive-orange.svg)

# Luna Resource Manager

Luna Resource Manager is a modular terminal file manager written in C. Its filesystem and process-management code uses POSIX APIs; its interactive terminal interface uses the curses library (ncurses, ncursesw, or a compatible implementation).

## Build requirements

- A C compiler with C99 support
- POSIX.1-2008-compatible system interfaces
- `make`
- A curses development library and headers
- libarchive development headers and library (for archive preview)
- `pkg-config` is optional; `CURSES_CFLAGS` and `CURSES_LIBS` can be supplied manually

On Debian or Ubuntu, install the build dependencies with:

```sh
sudo apt install build-essential libncurses-dev libarchive-dev pkg-config
```

Build and test:

```sh
make
make test
```

Run:

```sh
./bin/luna-resource-manager [directory]
./bin/luna-resource-manager --help
./bin/luna-resource-manager --config /path/to/config.conf /path/to/directory
```

## Configuration

Configuration is read from the first available location in this order: `--config FILE`, `LUNA_RESOURCE_MANAGER_CONFIG`, `$XDG_CONFIG_HOME/luna-resource-manager/config.conf` (or `$HOME/.config/luna-resource-manager/config.conf`), `/etc/luna-resource-manager.conf`, and `config/theme.conf` relative to the current directory.

Example:

```ini
theme=default
panel_ratio=55
show_header=1
show_preview=1
show_statusbar=1
```

Themes are `default`, `dark`, or `light`. Boolean values accept `0/1`, `yes/no`, `true/false`, or `on/off`. `panel_ratio` must be between 20 and 80.

## Key bindings

- Up / Down, Page Up / Page Down, Home / End: navigate
- Enter / Right: enter a directory or open a file
- Left: go to the parent directory
- F2 or mouse right-click: actions for the selected item
- Right-click a supported archive to open its preview screen directly; F2 → Open → Preview archive also works
- Archive preview lists entries in ZIP, TAR, compressed TAR, and 7z archives (using libarchive)
- `n`: create a directory
- `a`: About
- `?` / F1: Help
- `q`: quit

The built-in text viewer handles ordinary text files; executable files can be run with output captured in a bounded buffer. Editor, pager, and system-opener actions are launched without passing file paths through a shell. Set `LUNA_RESOURCE_MANAGER_OPEN` to choose a system opener program on systems where `xdg-open` or macOS `open` is unavailable.

## Source layout

- `app/main.c`: contains only `main()` and delegates to `app_run()`
- `app/app.c`: command-line handling and application startup
- `app/controller.c`: event loop, navigation, cursor and scroll state
- `app/config.c`: configuration parsing and theme selection
- `app/dialogs.c`, `app/menu.c`, `app/dir_picker.c`: terminal interaction components
- `app/fileops.c`: user-facing file operations
- `app/open.c`, `app/process.c`, `app/viewer.c`, `app/archive_preview.c`, `app/help.c`: file opening, process launching, viewing, and archive listing
- `app/state.c`, `app/callbacks.c`: application state and UI callbacks
- `core/fs.c`: directory listing, path joining and preview reads
- `core/fs_ops.c`: recursive copy, move and deletion
- `uidl/`: UI layout model and themes
- `prt/`: curses renderer
- `tests/test_fs.c`: filesystem regression tests

`make clean` removes generated binaries and object files. The source archive intentionally excludes build products.
