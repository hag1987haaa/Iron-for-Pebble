#pragma once
#include "../app_state.h"

void ui_intermediate_menu_create(Window *window, ActionBarLayer *action_bar, GColor main_bg, GColor main_fg);
void ui_intermediate_menu_destroy(void);
bool ui_intermediate_menu_is_active(void);
void ui_intermediate_menu_handle_up(void);
void ui_intermediate_menu_handle_down(void);
int ui_intermediate_menu_get_selected_idx(void);
void ui_intermediate_menu_set_selected_idx(int idx);
void ui_intermediate_menu_update_colors(GColor main_bg, GColor main_fg);
