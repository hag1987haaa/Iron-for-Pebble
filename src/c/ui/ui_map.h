#pragma once
#include "../app_state.h"
#include "../comm/graph_data.h"

void ui_map_init_buffers(const char *clock_buf, const char *h_buf, const char *m_buf, const char *s_buf, const bool *is_long_workout);
void ui_map_create(Window *window, GColor main_bg, GColor main_fg);
void ui_map_destroy(void);
bool ui_map_is_active(void);
void ui_map_update_data(const uint8_t *data, int length, int chunk_idx, int total_chunks);
void ui_map_mark_dirty(void);
void ui_map_update_colors(GColor main_bg, GColor main_fg);
