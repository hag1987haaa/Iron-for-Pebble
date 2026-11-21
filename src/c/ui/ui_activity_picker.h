#pragma once
#include "../app_state.h"

void ui_activity_picker_init(void);
void ui_activity_picker_save(void);
void ui_activity_picker_parse_and_set(const char *data_str);
int ui_activity_picker_get_count(void);
const ActivityItem* ui_activity_picker_get_activities(void);
bool ui_activity_picker_is_valid_id(uint8_t id);
const char* ui_activity_picker_get_name_by_id(uint8_t id);

void ui_activity_picker_create(Window *window, ActionBarLayer *action_bar, ActivityType initial_type, GColor main_bg, GColor main_fg);
void ui_activity_picker_destroy(void);
bool ui_activity_picker_is_active(void);
void ui_activity_picker_handle_up(void);
void ui_activity_picker_handle_down(void);
ActivityType ui_activity_picker_get_preview_activity(void);
void ui_activity_picker_set_preview_activity(ActivityType type);
uint8_t ui_activity_picker_get_selected_id(void);
void ui_activity_picker_set_selected_id(uint8_t id);
void ui_activity_picker_update_colors(GColor main_bg, GColor main_fg);
