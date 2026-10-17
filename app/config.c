#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stpui.h"
#include "stpui_ui.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char *trim(char *text)
{
    char *end;
    while (isspace((unsigned char)*text))
        ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        --end;
    *end = '\0';
    return text;
}

static int parse_boolean(const char *text, int *value)
{
    if (strcmp(text, "1") == 0 || strcmp(text, "yes") == 0 ||
        strcmp(text, "true") == 0 || strcmp(text, "on") == 0) {
        *value = 1;
        return 0;
    }
    if (strcmp(text, "0") == 0 || strcmp(text, "no") == 0 ||
        strcmp(text, "false") == 0 || strcmp(text, "off") == 0) {
        *value = 0;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

void config_defaults(AppConfig *config)
{
    if (config == NULL)
        return;
    memset(config, 0, sizeof(*config));
    config->panel_ratio = 55;
    config->show_header = 1;
    config->show_statusbar = 1;
    config->show_preview = 1;
    (void)snprintf(config->theme, sizeof(config->theme), "default");
}

static int config_assign(AppConfig *config, const char *key, const char *value)
{
    char *end;
    long number;
    int boolean;
    if (strcmp(key, "theme") == 0) {
        if (strcmp(value, "default") != 0 && strcmp(value, "dark") != 0 &&
            strcmp(value, "light") != 0) {
            errno = EINVAL;
            return -1;
        }
        (void)snprintf(config->theme, sizeof(config->theme), "%s", value);
        return 0;
    }
    if (strcmp(key, "panel_ratio") == 0) {
        errno = 0;
        number = strtol(value, &end, 10);
        if (errno != 0 || *value == '\0' || *trim(end) != '\0' || number < 20 || number > 80) {
            errno = EINVAL;
            return -1;
        }
        config->panel_ratio = (int)number;
        return 0;
    }
    if (strcmp(key, "show_header") == 0 || strcmp(key, "show_statusbar") == 0 ||
        strcmp(key, "show_preview") == 0) {
        if (parse_boolean(value, &boolean) != 0)
            return -1;
        if (strcmp(key, "show_header") == 0)
            config->show_header = boolean;
        else if (strcmp(key, "show_statusbar") == 0)
            config->show_statusbar = boolean;
        else
            config->show_preview = boolean;
        return 0;
    }
    /* Unknown keys are ignored to allow forward-compatible config files. */
    return 0;
}

static int config_read_file(AppConfig *config, const char *path)
{
    FILE *stream;
    char line[1024];
    unsigned long line_number = 0;

    if (path == NULL || path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    stream = fopen(path, "r");
    if (stream == NULL)
        return -1;
    while (fgets(line, sizeof(line), stream) != NULL) {
        char *text;
        char *equals;
        char *key;
        char *value;
        ++line_number;
        if (strlen(line) == sizeof(line) - 1 && line[sizeof(line) - 2] != '\n' && !feof(stream)) {
            (void)fprintf(stderr, "%s:%lu: configuration line is too long\n", path, line_number);
            errno = EOVERFLOW;
            fclose(stream);
            return -1;
        }
        text = trim(line);
        if (*text == '\0' || *text == '#')
            continue;
        equals = strchr(text, '=');
        if (equals == NULL) {
            (void)fprintf(stderr, "%s:%lu: expected key=value\n", path, line_number);
            errno = EINVAL;
            fclose(stream);
            return -1;
        }
        *equals = '\0';
        key = trim(text);
        value = trim(equals + 1);
        if (config_assign(config, key, value) != 0) {
            int saved_errno = errno != 0 ? errno : EINVAL;
            (void)fprintf(stderr, "%s:%lu: invalid value for %s\n", path, line_number, key);
            fclose(stream);
            errno = saved_errno;
            return -1;
        }
    }
    if (ferror(stream)) {
        int saved_errno = errno != 0 ? errno : EIO;
        fclose(stream);
        errno = saved_errno;
        return -1;
    }
    return fclose(stream);
}

static int try_config_path(AppConfig *config, const char *path, int required)
{
    if (path == NULL || path[0] == '\0')
        return 0;
    if (config_read_file(config, path) == 0) {
        (void)snprintf(config->config_path, sizeof(config->config_path), "%s", path);
        return 1;
    }
    if (errno == ENOENT && !required)
        return 0;
    return -1;
}

int config_load(AppConfig *config, const char *explicit_path)
{
    const char *environment_path;
    const char *xdg;
    const char *home;
    char path[PATH_MAX];
    int result;

    if (config == NULL) {
        errno = EINVAL;
        return -1;
    }
    config_defaults(config);
    if (explicit_path != NULL)
        return try_config_path(config, explicit_path, 1) == 1 ? 0 : -1;

    environment_path = getenv("LUNA_RESOURCE_MANAGER_CONFIG");
    if (environment_path != NULL && *environment_path != '\0')
        return try_config_path(config, environment_path, 1) == 1 ? 0 : -1;

    xdg = getenv("XDG_CONFIG_HOME");
    home = getenv("HOME");
    if (xdg != NULL && *xdg != '\0') {
        if (snprintf(path, sizeof(path), "%s/luna-resource-manager/config.conf", xdg) < (int)sizeof(path)) {
            result = try_config_path(config, path, 0);
            if (result != 0)
                return result < 0 ? -1 : 0;
        }
    }
    if (home != NULL && *home != '\0') {
        if (snprintf(path, sizeof(path), "%s/.config/luna-resource-manager/config.conf", home) < (int)sizeof(path)) {
            result = try_config_path(config, path, 0);
            if (result != 0)
                return result < 0 ? -1 : 0;
        }
    }
    result = try_config_path(config, "/etc/luna-resource-manager.conf", 0);
    if (result != 0)
        return result < 0 ? -1 : 0;
    result = try_config_path(config, "config/theme.conf", 0);
    return result < 0 ? -1 : 0;
}

void config_apply_theme(const AppConfig *config)
{
    StpTheme theme;
    if (config == NULL)
        return;
    if (strcmp(config->theme, "dark") == 0)
        stpui_theme_dark(&theme);
    else if (strcmp(config->theme, "light") == 0)
        stpui_theme_light(&theme);
    else
        stpui_theme_default(&theme);
    stpui_set_theme(&theme);
}

void config_print_help(const char *program)
{
    (void)printf("Usage: %s [options] [directory]\n\n", program != NULL ? program : "luna-resource-manager");
    (void)printf("Options:\n");
    (void)printf("  -h, --help             Show this help\n");
    (void)printf("  -V, --version          Show version\n");
    (void)printf("  -c, --config FILE      Read configuration from FILE\n\n");
    (void)printf("Config keys: theme=default|dark|light, panel_ratio=20..80,\n");
    (void)printf("show_header=0|1, show_statusbar=0|1, show_preview=0|1\n");
}

void config_print_version(void)
{
    (void)printf("Luna Resource Manager 1.1.0 (POSIX-oriented C file manager)\n");
}
