#pragma once
#include "../app_state.h"

void ui_marquee_init(Window *window, ActionBarLayer *action_bar, Layer *graph_layer);
void ui_marquee_trigger(uint8_t app_state, GColor fg_color, GColor bg_color);
void ui_marquee_trigger_custom(const char *msg, GColor fg_color, GColor bg_color, uint8_t app_state);
void ui_marquee_stop(void);
void ui_marquee_destroy(void);
bool ui_marquee_is_active(void);
bool ui_marquee_is_custom(void);
void ui_marquee_set_action_bar(ActionBarLayer *action_bar);
void ui_marquee_set_graph_layer(Layer *graph_layer);
