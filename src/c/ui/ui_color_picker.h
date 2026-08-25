#pragma once
#include "../app_state.h"

#if defined(PBL_COLOR)
void ui_color_picker_create(Window *window, ActionBarLayer *action_bar, GColor main_bg, GColor main_fg);
void ui_color_picker_destroy(void);
bool ui_color_picker_is_active(void);
void ui_color_picker_handle_down(void);
int ui_color_picker_get_selected_idx(void);
void ui_color_picker_set_selected_idx(int idx);
uint8_t ui_color_picker_get_personal_color_argb(void);
void ui_color_picker_toggle_preview(void);
bool ui_color_picker_get_preview_is_running(void);
void ui_color_picker_update_colors(GColor main_bg, GColor main_fg);
#endif
