#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app/config.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        (void)fprintf(stderr, "test_config: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    char valid_path[] = "/tmp/luna-resource-manager-config.XXXXXX";
    char invalid_path[] = "/tmp/luna-resource-manager-invalid.XXXXXX";
    int valid_fd = mkstemp(valid_path);
    int invalid_fd = mkstemp(invalid_path);
    FILE *stream;
    AppConfig config;

    require(valid_fd >= 0 && invalid_fd >= 0, "create temporary config files");
    (void)close(valid_fd);
    (void)close(invalid_fd);

    stream = fopen(valid_path, "w");
    require(stream != NULL, "open valid config file");
    require(fputs("theme=dark\npanel_ratio=62\nshow_header=no\nshow_preview=0\nshow_statusbar=on\n", stream) >= 0,
            "write valid config file");
    require(fclose(stream) == 0, "close valid config file");
    require(config_load(&config, valid_path) == 0, "load valid config");
    require(strcmp(config.theme, "dark") == 0, "parse theme");
    require(config.panel_ratio == 62, "parse panel ratio");
    require(config.show_header == 0, "parse no boolean");
    require(config.show_preview == 0, "parse numeric boolean");
    require(config.show_statusbar == 1, "parse on boolean");
    require(strcmp(config.config_path, valid_path) == 0, "remember config path");

    stream = fopen(invalid_path, "w");
    require(stream != NULL, "open invalid config file");
    require(fputs("panel_ratio=101\n", stream) >= 0, "write invalid config file");
    require(fclose(stream) == 0, "close invalid config file");
    errno = 0;
    require(config_load(&config, invalid_path) == -1, "reject invalid panel ratio");
    require(errno == EINVAL, "report invalid config value");

    require(unlink(valid_path) == 0, "remove valid config file");
    require(unlink(invalid_path) == 0, "remove invalid config file");
    (void)puts("test_config: all tests passed");
    return 0;
}
