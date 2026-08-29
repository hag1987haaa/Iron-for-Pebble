#pragma once
#include "../app_state.h"

void graph_data_init(void);
void graph_data_clear(void);
void graph_data_save_to_persist(void);
void graph_data_load_from_persist(ActivityType *out_activity, int *out_color_idx);
void graph_data_parse_hybrid(const char *input);
void graph_data_parse_mid(const char *input);
void graph_data_parse_lower(const char *input);
bool graph_data_is_detail_mode(uint8_t app_state);

int graph_data_get_count(void);
int graph_data_get_id(void);
int graph_data_get_scale(void);
int graph_data_get_point(int index);

const char* graph_data_get_y_label(void);
const char* graph_data_get_x_label(void);
const char* graph_data_get_max_label(void);
const char* graph_data_get_min_label(void);

// 中段データ
int graph_data_get_mid_page_count(void);
int graph_data_get_current_mid_mode(void);
void graph_data_set_current_mid_mode(int mode);
bool graph_data_select_mid_by_id(int id);
const MidPageData* graph_data_get_mid_page(int index);
const MidPageData* graph_data_get_current_mid_page(void);

// 下段データ
int graph_data_get_lower_page_count(void);
int graph_data_get_current_lower_mode(void);
void graph_data_set_current_lower_mode(int mode);
bool graph_data_select_lower_by_id(int id);
const LowerPageData* graph_data_get_lower_page(int index);
const LowerPageData* graph_data_get_current_lower_page(void);
bool graph_data_is_lower_graph_mode(void);

bool graph_data_is_id_protocol_active(void);
