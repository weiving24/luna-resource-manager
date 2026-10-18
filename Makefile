PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SYSCONFDIR ?= /etc
CC ?= cc

PROGRAM_NAME = Luna Resource Manager
VERSION = 0.1.2

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -I. -Iinclude -Iuidl -Iprt -Icore -Iapp \
            '-DPROGRAM_NAME="$(PROGRAM_NAME)"' '-DLUNA_VERSION="$(VERSION)"'

CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic
LDFLAGS ?=
CURSES_CFLAGS ?=
CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || pkg-config --libs ncurses 2>/dev/null || printf '%s' '-lcurses')
ARCHIVE_CFLAGS ?= $(shell pkg-config --cflags libarchive 2>/dev/null)
ARCHIVE_LIBS ?= $(shell pkg-config --libs libarchive 2>/dev/null || printf '%s' '-larchive')
LDLIBS += $(CURSES_LIBS) $(ARCHIVE_LIBS)

PROGRAM = bin/luna-resource-manager
SOURCES = \
	app/main.c \
	app/app.c \
	app/config.c \
	app/controller.c \
	app/state.c \
	app/callbacks.c \
	app/dialogs.c \
	app/menu.c \
	app/dir_picker.c \
	app/fileops.c \
	app/open.c \
	app/archive_preview.c \
	app/process.c \
	app/viewer.c \
	app/help.c \
	core/fs.c \
	core/fs_ops.c \
	uidl/stpui.c \
	uidl/theme.c \
	prt/stpprt.c
OBJECTS = $(SOURCES:%.c=build/%.o)

.PHONY: all clean test install uninstall

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS) | bin
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CURSES_CFLAGS) $(ARCHIVE_CFLAGS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p $@

build/tests/test_fs: tests/test_fs.c core/fs.c core/fs_ops.c core/fs.h core/fs_ops.h
	@mkdir -p build/tests
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_fs.c core/fs.c core/fs_ops.c

build/tests/test_config: tests/test_config.c app/config.c uidl/stpui.c uidl/theme.c
	@mkdir -p build/tests
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_config.c app/config.c uidl/stpui.c uidl/theme.c

test: build/tests/test_fs build/tests/test_config
	./build/tests/test_fs
	./build/tests/test_config

install: all
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(SYSCONFDIR)
	cp $(PROGRAM) $(DESTDIR)$(BINDIR)/luna-resource-manager
	cp config/theme.conf $(DESTDIR)$(SYSCONFDIR)/luna-resource-manager.conf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/luna-resource-manager $(DESTDIR)$(SYSCONFDIR)/luna-resource-manager.conf

clean:
	rm -rf build bin
