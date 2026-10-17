#ifndef LUNA_RESOURCE_MANAGER_RENDERER_H
#define LUNA_RESOURCE_MANAGER_RENDERER_H

#include "stpui.h"

int stpprt_init(void);
void stpprt_shutdown(void);
void stpprt_resize(int width, int height);
void stpprt_draw_frame(const UiFrame *frame);
void stpprt_clear(void);
void stpprt_get_size(int *width, int *height);
int stpprt_read_key(void);

#endif
