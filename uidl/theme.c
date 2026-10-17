#include "stpui_theme.h"

#include <stddef.h>

void stpui_theme_default(StpTheme *theme)
{
    if (theme == NULL)
        return;
    theme->header = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_CYAN, 1, 0, 0 };
    theme->status = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_BRIGHT_BLACK, 0, 0, 0 };
    theme->dir = (StpStyle){ STP_COLOR_BLUE, STP_COLOR_DEFAULT, 1, 0, 0 };
    theme->file = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->focus = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_GREEN, 0, 0, 0 };
    theme->separator = (StpStyle){ STP_COLOR_BRIGHT_BLACK, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->input = (StpStyle){ STP_COLOR_YELLOW, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->preview = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0 };
}

void stpui_theme_dark(StpTheme *theme)
{
    if (theme == NULL)
        return;
    theme->header = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_BLUE, 1, 0, 0 };
    theme->status = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_BLACK, 0, 0, 0 };
    theme->dir = (StpStyle){ STP_COLOR_CYAN, STP_COLOR_DEFAULT, 1, 0, 0 };
    theme->file = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->focus = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_YELLOW, 0, 0, 0 };
    theme->separator = (StpStyle){ STP_COLOR_BRIGHT_BLACK, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->input = (StpStyle){ STP_COLOR_GREEN, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->preview = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_DEFAULT, 0, 0, 0 };
}

void stpui_theme_light(StpTheme *theme)
{
    if (theme == NULL)
        return;
    theme->header = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_WHITE, 1, 0, 0 };
    theme->status = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_BRIGHT_WHITE, 0, 0, 0 };
    theme->dir = (StpStyle){ STP_COLOR_BLUE, STP_COLOR_DEFAULT, 1, 0, 0 };
    theme->file = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->focus = (StpStyle){ STP_COLOR_WHITE, STP_COLOR_BLUE, 0, 0, 0 };
    theme->separator = (StpStyle){ STP_COLOR_BRIGHT_BLACK, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->input = (StpStyle){ STP_COLOR_MAGENTA, STP_COLOR_DEFAULT, 0, 0, 0 };
    theme->preview = (StpStyle){ STP_COLOR_BLACK, STP_COLOR_DEFAULT, 0, 0, 0 };
}
