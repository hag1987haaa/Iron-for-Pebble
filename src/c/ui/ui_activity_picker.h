#pragma once
#include "../app_state.h"

void ui_activity_picker_create(Window *window, ActionBarLayer *action_bar, ActivityType initial_type, GColor main_bg, GColor main_fg);
void ui_activity_picker_destroy(void);
bool ui_activity_picker_is_active(void);
void ui_activity_picker_handle_up(void);
void ui_activity_picker_handle_down(void);
ActivityType ui_activity_picker_get_preview_activity(void);
void ui_activity_picker_set_preview_activity(ActivityType type);
void ui_activity_picker_update_colors(GColor main_bg, GColor main_fg);
