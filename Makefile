PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SYSCONFDIR ?= /etc

CC ?= cc
PROGRAM_NAME = Luna Resource Manager (ckresmgr)
VERSION = 0.1.3-rc1

GREEN  := \033[32m
CYAN   := \033[36m
YELLOW := \033[33m
MAGENTA:= \033[35m
BRIGHT := \033[1;32m
RESET  := \033[0m
INFO   = printf '$(1)%s$(RESET) %s\n' '$(2)' '$(3)'

CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
            -I. -Iinclude -Iuidl -Iprt -Icore -Iapp \
            '-DPROGRAM_NAME="$(PROGRAM_NAME)"' \
            '-DLUNA_VERSION="$(VERSION)"'

CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic
LDFLAGS ?=

CURSES_CFLAGS ?=
CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || \
                      pkg-config --libs ncurses 2>/dev/null || \
                      printf '%s' '-lcurses')

ARCHIVE_CFLAGS ?= $(shell pkg-config --cflags libarchive 2>/dev/null)
ARCHIVE_LIBS ?= $(shell pkg-config --libs libarchive 2>/dev/null || \
                    printf '%s' '-larchive')

LDLIBS += $(CURSES_LIBS) $(ARCHIVE_LIBS)

PROGRAM = bin/ckresmgr

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
    app/easter_egg.c \
    core/fs.c \
    core/fs_ops.c \
    uidl/stpui.c \
    uidl/theme.c \
    prt/stpprt.c

.PHONY: all clean test install uninstall

all: $(PROGRAM)

$(PROGRAM): $(SOURCES) | bin
	@$(call INFO,$(YELLOW),[BUILD],$@)
	@$(CC) $(CURSES_CFLAGS) $(ARCHIVE_CFLAGS) $(CPPFLAGS) $(CFLAGS) \
		$(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)
	@$(call INFO,$(BRIGHT),[OUTPUT],$(notdir $@))

bin:
	@mkdir -p $@

build/tests/test_fs: tests/test_fs.c core/fs.c core/fs_ops.c core/fs.h core/fs_ops.h
	@mkdir -p build/tests
	@$(call INFO,$(GREEN),[TEST CC],$<)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
		tests/test_fs.c core/fs.c core/fs_ops.c

build/tests/test_config: tests/test_config.c app/config.c uidl/stpui.c uidl/theme.c
	@mkdir -p build/tests
	@$(call INFO,$(GREEN),[TEST CC],$<)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
		tests/test_config.c app/config.c uidl/stpui.c uidl/theme.c

test: build/tests/test_fs build/tests/test_config
	./build/tests/test_fs
	./build/tests/test_config

install: all
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(SYSCONFDIR)
	cp $(PROGRAM) $(DESTDIR)$(BINDIR)/luna-resource-manager
	cp config/theme.conf $(DESTDIR)$(SYSCONFDIR)/luna-resource-manager.conf

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/luna-resource-manager
	rm -f $(DESTDIR)$(SYSCONFDIR)/luna-resource-manager.conf

clean:
	rm -rf build bin
