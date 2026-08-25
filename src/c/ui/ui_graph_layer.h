#pragma once
#include "../app_state.h"

void ui_graph_layer_update_proc(Layer *layer, GContext *ctx, uint8_t app_state, GColor fg_color);
void ui_mid_bg_layer_update_proc(Layer *layer, GContext *ctx, uint8_t app_state, GColor main_bg, GColor main_fg, bool is_paused, int current_hr);
