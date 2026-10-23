#define _POSIX_C_SOURCE 200809L

#include "stpui_theme.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define THEME_LINE_MAX 1024

static StpStyle style_make(StpColor fg, StpColor bg,
                           int bold, int reverse, int underline)
{
    StpStyle s;
    memset(&s, 0, sizeof(s));
    s.fg = fg;
    s.bg = bg;
    s.bold = (uint8_t)!!bold;
    s.reverse = (uint8_t)!!reverse;
    s.underline = (uint8_t)!!underline;
    return s;
}

void stpui_theme_default(StpTheme *theme)
{
    if (theme == NULL)
        return;
    theme->header = style_make(STP_COLOR_BLACK, STP_COLOR_CYAN, 1, 0, 0);
    theme->status = style_make(STP_COLOR_WHITE, STP_COLOR_BRIGHT_BLACK, 0, 0, 0);
    theme->dir = style_make(STP_COLOR_BLUE, STP_COLOR_DEFAULT, 1, 0, 0);
    theme->file = style_make(STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->focus = style_make(STP_COLOR_BLACK, STP_COLOR_GREEN, 0, 0, 0);
    theme->separator = style_make(STP_COLOR_BRIGHT_BLACK, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->input = style_make(STP_COLOR_YELLOW, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->preview = style_make(STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0);
}

void stpui_theme_dark(StpTheme *theme)
{
    if (theme == NULL)
        return;
    stpui_theme_default(theme);
    theme->header = style_make(STP_COLOR_WHITE, STP_COLOR_BLACK, 1, 0, 0);
    theme->status = style_make(STP_COLOR_WHITE, STP_COLOR_BLACK, 0, 0, 0);
    theme->dir = style_make(STP_COLOR_CYAN, STP_COLOR_DEFAULT, 1, 0, 0);
    theme->file = style_make(STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->focus = style_make(STP_COLOR_BLACK, STP_COLOR_CYAN, 1, 0, 0);
    theme->separator = style_make(STP_COLOR_BRIGHT_BLACK, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->input = style_make(STP_COLOR_YELLOW, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->preview = style_make(STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0);
}

void stpui_theme_light(StpTheme *theme)
{
    if (theme == NULL)
        return;
    stpui_theme_default(theme);
    theme->header = style_make(STP_COLOR_WHITE, STP_COLOR_BLUE, 1, 0, 0);
    theme->status = style_make(STP_COLOR_BLACK, STP_COLOR_BRIGHT_WHITE, 0, 0, 0);
    theme->dir = style_make(STP_COLOR_BLUE, STP_COLOR_DEFAULT, 1, 0, 0);
    theme->file = style_make(STP_COLOR_BLACK, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->focus = style_make(STP_COLOR_WHITE, STP_COLOR_BLUE, 1, 0, 0);
    theme->separator = style_make(STP_COLOR_BRIGHT_BLACK, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->input = style_make(STP_COLOR_BLUE, STP_COLOR_DEFAULT, 0, 0, 0);
    theme->preview = style_make(STP_COLOR_BLACK, STP_COLOR_DEFAULT, 0, 0, 0);
}

static void set_rgb(StpStyle *s, uint32_t fg, uint32_t bg,
                    int use_fg, int use_bg, int bold)
{
    s->fg_rgb = fg;
    s->bg_rgb = bg;
    s->use_fg_rgb = (uint8_t)!!use_fg;
    s->use_bg_rgb = (uint8_t)!!use_bg;
    s->bold = (uint8_t)!!bold;
}

int stpui_theme_named(const char *name, StpTheme *theme)
{
    if (name == NULL || theme == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strcasecmp(name, "default") == 0) {
        stpui_theme_default(theme);
        return 0;
    }
    if (strcasecmp(name, "dark") == 0) {
        stpui_theme_dark(theme);
        return 0;
    }
    if (strcasecmp(name, "light") == 0) {
        stpui_theme_light(theme);
        return 0;
    }

    stpui_theme_dark(theme);

    if (strcasecmp(name, "nord") == 0) {
        set_rgb(&theme->header, 0xECEFF4, 0x3B4252, 1, 1, 1);
        set_rgb(&theme->status, 0xD8DEE9, 0x2E3440, 1, 1, 0);
        set_rgb(&theme->dir, 0x88C0D0, 0, 1, 0, 1);
        set_rgb(&theme->file, 0xD8DEE9, 0, 1, 0, 0);
        set_rgb(&theme->focus, 0x2E3440, 0x88C0D0, 1, 1, 1);
        set_rgb(&theme->separator, 0x616E88, 0, 1, 0, 0);
        set_rgb(&theme->input, 0xA3BE8C, 0x2E3440, 1, 1, 0);
        set_rgb(&theme->preview, 0xE5E9F0, 0x2E3440, 1, 1, 0);
        return 0;
    }
    if (strcasecmp(name, "dracula") == 0) {
        set_rgb(&theme->header, 0xF8F8F2, 0x44475A, 1, 1, 1);
        set_rgb(&theme->status, 0xF8F8F2, 0x282A36, 1, 1, 0);
        set_rgb(&theme->dir, 0x8BE9FD, 0, 1, 0, 1);
        set_rgb(&theme->file, 0xF8F8F2, 0, 1, 0, 0);
        set_rgb(&theme->focus, 0x282A36, 0xBD93F9, 1, 1, 1);
        set_rgb(&theme->separator, 0x6272A4, 0, 1, 0, 0);
        set_rgb(&theme->input, 0x50FA7B, 0x282A36, 1, 1, 0);
        set_rgb(&theme->preview, 0xF1FA8C, 0x282A36, 1, 1, 0);
        return 0;
    }
    if (strcasecmp(name, "monokai") == 0) {
        set_rgb(&theme->header, 0xF8F8F2, 0x49483E, 1, 1, 1);
        set_rgb(&theme->status, 0xF8F8F2, 0x272822, 1, 1, 0);
        set_rgb(&theme->dir, 0xA6E22E, 0, 1, 0, 1);
        set_rgb(&theme->file, 0xF8F8F2, 0, 1, 0, 0);
        set_rgb(&theme->focus, 0x272822, 0xFD971F, 1, 1, 1);
        set_rgb(&theme->separator, 0x75715E, 0, 1, 0, 0);
        set_rgb(&theme->input, 0x66D9EF, 0x272822, 1, 1, 0);
        set_rgb(&theme->preview, 0xE6DB74, 0x272822, 1, 1, 0);
        return 0;
    }
    if (strcasecmp(name, "solarized-dark") == 0) {
        set_rgb(&theme->header, 0xEEE8D5, 0x073642, 1, 1, 1);
        set_rgb(&theme->status, 0x93A1A1, 0x002B36, 1, 1, 0);
        set_rgb(&theme->dir, 0x268BD2, 0, 1, 0, 1);
        set_rgb(&theme->file, 0x839496, 0, 1, 0, 0);
        set_rgb(&theme->focus, 0x002B36, 0xB58900, 1, 1, 1);
        set_rgb(&theme->separator, 0x586E75, 0, 1, 0, 0);
        set_rgb(&theme->input, 0x2AA198, 0x002B36, 1, 1, 0);
        set_rgb(&theme->preview, 0xEEE8D5, 0x002B36, 1, 1, 0);
        return 0;
    }
    if (strcasecmp(name, "gruvbox") == 0) {
        set_rgb(&theme->header, 0xEBDBB2, 0x3C3836, 1, 1, 1);
        set_rgb(&theme->status, 0xD5C4A1, 0x282828, 1, 1, 0);
        set_rgb(&theme->dir, 0x83A598, 0, 1, 0, 1);
        set_rgb(&theme->file, 0xEBDBB2, 0, 1, 0, 0);
        set_rgb(&theme->focus, 0x282828, 0xFABD2F, 1, 1, 1);
        set_rgb(&theme->separator, 0x665C54, 0, 1, 0, 0);
        set_rgb(&theme->input, 0xB8BB26, 0x282828, 1, 1, 0);
        set_rgb(&theme->preview, 0xFE8019, 0x282828, 1, 1, 0);
        return 0;
    }
    if (strcasecmp(name, "pink") == 0) {
        set_rgb(&theme->header,    0xFFF0F6, 0xD94683, 1, 1, 1);
        set_rgb(&theme->status,    0xFFE4EF, 0x35172B, 1, 1, 0);
        set_rgb(&theme->dir,       0xFF79B0, 0x241526, 1, 1, 1);
        set_rgb(&theme->file,      0xFFF0F6, 0x241526, 1, 1, 0);
        set_rgb(&theme->focus,     0x35172B, 0xFF8FC7, 1, 1, 1);
        set_rgb(&theme->separator, 0xC084A5, 0x241526, 1, 1, 0);
        set_rgb(&theme->input,     0xFFD1E5, 0x35172B, 1, 1, 0);
        set_rgb(&theme->preview,   0xFFF0F6, 0x241526, 1, 1, 0);
        return 0;
    }

    if (strcasecmp(name, "purple") == 0) {
        set_rgb(&theme->header, 0xFFFFFF, 0x6D28D9, 1, 1, 1);
        set_rgb(&theme->status, 0xE2E8F0, 0x1E1B4B, 1, 1, 0);
        set_rgb(&theme->dir, 0x38BDF8, 0, 1, 0, 1);
        set_rgb(&theme->file, 0xCBD5E1, 0, 1, 0, 0);
        set_rgb(&theme->focus, 0xFFFFFF, 0xEA580C, 1, 1, 1);
        set_rgb(&theme->separator, 0x64748B, 0, 1, 0, 0);
        set_rgb(&theme->input, 0xA7F3D0, 0x064E3B, 1, 1, 0);
        set_rgb(&theme->preview, 0xFDE68A, 0x111827, 1, 1, 0);
        return 0;
    }

    errno = ENOENT;
    return -1;
}

static char *trim(char *s)
{
    char *end;
    while (isspace((unsigned char)*s))
        ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        --end;
    *end = '\0';
    return s;
}

static int parse_bool(const char *s, uint8_t *out)
{
    if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0 ||
        strcasecmp(s, "yes") == 0 || strcasecmp(s, "on") == 0) {
        *out = 1;
        return 0;
    }
    if (strcasecmp(s, "false") == 0 || strcmp(s, "0") == 0 ||
        strcasecmp(s, "no") == 0 || strcasecmp(s, "off") == 0) {
        *out = 0;
        return 0;
    }
    return -1;
}

static int parse_color(const char *value, StpColor *basic,
                       uint32_t *rgb, uint8_t *use_rgb)
{
    static const struct {
        const char *name;
        StpColor color;
    } colors[] = {
        { "default", STP_COLOR_DEFAULT }, { "black", STP_COLOR_BLACK },
        { "red", STP_COLOR_RED }, { "green", STP_COLOR_GREEN },
        { "yellow", STP_COLOR_YELLOW }, { "blue", STP_COLOR_BLUE },
        { "magenta", STP_COLOR_MAGENTA }, { "cyan", STP_COLOR_CYAN },
        { "white", STP_COLOR_WHITE }, { "bright_black", STP_COLOR_BRIGHT_BLACK },
        { "bright-white", STP_COLOR_BRIGHT_WHITE }, { "bright_white", STP_COLOR_BRIGHT_WHITE },
        { "transparent", STP_COLOR_DEFAULT }
    };
    size_t i;
    unsigned int r, g, b;
    char tail;

    if (value == NULL)
        return -1;
    if (value[0] == '#' && strlen(value) == 7 &&
        sscanf(value + 1, "%2x%2x%2x%c", &r, &g, &b, &tail) == 3) {
        *rgb = (r << 16) | (g << 8) | b;
        *use_rgb = 1;
        return 0;
    }
    for (i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
        if (strcasecmp(value, colors[i].name) == 0) {
            *basic = colors[i].color;
            *use_rgb = 0;
            return 0;
        }
    }
    return -1;
}

static StpStyle *section_style(StpTheme *theme, const char *section)
{
    if (strcasecmp(section, "header") == 0) return &theme->header;
    if (strcasecmp(section, "status") == 0 ||
        strcasecmp(section, "statusbar") == 0) return &theme->status;
    if (strcasecmp(section, "dir") == 0 ||
        strcasecmp(section, "directory") == 0) return &theme->dir;
    if (strcasecmp(section, "file") == 0) return &theme->file;
    if (strcasecmp(section, "focus") == 0) return &theme->focus;
    if (strcasecmp(section, "separator") == 0) return &theme->separator;
    if (strcasecmp(section, "input") == 0) return &theme->input;
    if (strcasecmp(section, "preview") == 0) return &theme->preview;
    return NULL;
}

int stpui_theme_load(const char *path, StpTheme *theme)
{
    FILE *fp;
    char line[THEME_LINE_MAX];
    char section[64] = "";
    StpTheme parsed;
    unsigned long line_no = 0;

    if (path == NULL || theme == NULL) {
        errno = EINVAL;
        return -1;
    }
    fp = fopen(path, "r");
    if (fp == NULL)
        return -1;

    parsed = *theme;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *p, *eq, *key, *value;
        StpStyle *style;
        uint8_t boolean;
        StpColor basic = STP_COLOR_DEFAULT;
        uint32_t rgb = 0;
        uint8_t use_rgb = 0;

        ++line_no;
        if (strchr(line, '\n') == NULL && !feof(fp)) {
            fclose(fp);
            errno = EOVERFLOW;
            return -1;
        }
        p = trim(line);
        if (*p == '\0' || *p == '#' || *p == ';')
            continue;

        if (*p == '[') {
            char *close = strchr(p + 1, ']');
            if (close == NULL || *trim(close + 1) != '\0' ||
                (size_t)(close - p - 1) >= sizeof(section)) {
                fclose(fp);
                errno = EINVAL;
                return -1;
            }
            *close = '\0';
            strcpy(section, trim(p + 1));
            if (section_style(&parsed, section) == NULL) {
                fclose(fp);
                errno = EINVAL;
                return -1;
            }
            continue;
        }

        eq = strchr(p, '=');
        if (eq == NULL || section[0] == '\0') {
            fclose(fp);
            errno = EINVAL;
            return -1;
        }
        *eq = '\0';
        key = trim(p);
        value = trim(eq + 1);
        style = section_style(&parsed, section);
        if (style == NULL || *key == '\0' || *value == '\0') {
            fclose(fp);
            errno = EINVAL;
            return -1;
        }

        if (strcasecmp(key, "fg") == 0 ||
            strcasecmp(key, "foreground") == 0) {
            if (parse_color(value, &basic, &rgb, &use_rgb) != 0) {
                fclose(fp); errno = EINVAL; return -1;
            }
            style->fg = basic;
            style->fg_rgb = rgb;
            style->use_fg_rgb = use_rgb;
        } else if (strcasecmp(key, "bg") == 0 ||
                   strcasecmp(key, "background") == 0) {
            if (parse_color(value, &basic, &rgb, &use_rgb) != 0) {
                fclose(fp); errno = EINVAL; return -1;
            }
            style->bg = basic;
            style->bg_rgb = rgb;
            style->use_bg_rgb = use_rgb;
        } else if (strcasecmp(key, "bold") == 0) {
            if (parse_bool(value, &boolean) != 0) {
                fclose(fp); errno = EINVAL; return -1;
            }
            style->bold = boolean;
        } else if (strcasecmp(key, "reverse") == 0) {
            if (parse_bool(value, &boolean) != 0) {
                fclose(fp); errno = EINVAL; return -1;
            }
            style->reverse = boolean;
        } else if (strcasecmp(key, "underline") == 0) {
            if (parse_bool(value, &boolean) != 0) {
                fclose(fp); errno = EINVAL; return -1;
            }
            style->underline = boolean;
        } else {
            fclose(fp);
            errno = EINVAL;
            return -1;
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    if (fclose(fp) != 0)
        return -1;

    *theme = parsed;
    (void)line_no;
    return 0;
}
