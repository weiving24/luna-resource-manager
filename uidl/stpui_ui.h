#ifndef LUNA_RESOURCE_MANAGER_UI_LAYOUT_H
#define LUNA_RESOURCE_MANAGER_UI_LAYOUT_H

#include "stpui.h"

int stpui_init(const LayoutParams *params, const SetupCallbacks *callbacks);
void stpui_shutdown(void);
void stpui_set_layout(const LayoutParams *params);
void stpui_resize(int width, int height);
void stpui_set_theme(const StpTheme *theme);
UiFrame *stpui_build_frame(const char *cwd, int cursor, int scroll,
                           const char *search_text, const char *status_msg);
void stpui_frame_free(UiFrame *frame);
int stpui_list_count(const UiFrame *frame, const char *tag);
int stpui_list_cursor(const UiFrame *frame, const char *tag);
int stpui_list_scroll(const UiFrame *frame, const char *tag);

#endif
