#include "graph_data.h"

static int s_graph_data[MAX_GRAPH_DATA]; 
static int s_graph_count = 0, s_graph_id = 0, s_graph_scale = 1; 

static MidPageData s_mid_pages[MAX_MID_PAGES];
static int s_mid_page_count = 0;
static int s_current_mid_mode = 0;

static LowerPageData s_lower_pages[MAX_LOWER_PAGES];
static int s_lower_page_count = 0;
static int s_current_lower_mode = 0;

static char s_graph_y_label[16] = "";
static char s_graph_x_label[16] = "";
static char s_graph_max_label[16] = "";
static char s_graph_min_label[16] = "";

void graph_data_init(void) {
    s_graph_count = 0;
    s_graph_id = 0;
    s_graph_scale = 1;
    memset(s_graph_data, 0, sizeof(s_graph_data));
    s_mid_page_count = 0;
    s_current_mid_mode = 0;
    s_lower_page_count = 0;
    s_current_lower_mode = 0;
}

void graph_data_save_to_persist(void) {
    persist_write_int(PK_GRAPH_COUNT, s_graph_count);
    persist_write_data(PK_GRAPH_DATA, s_graph_data, sizeof(s_graph_data));
    persist_write_int(PK_GRAPH_ID, s_graph_id);
    persist_write_int(PK_GRAPH_SCALE, s_graph_scale);
}

void graph_data_load_from_persist(ActivityType *out_activity, int *out_color_idx) {
    if (persist_exists(PK_GRAPH_COUNT)) s_graph_count = persist_read_int(PK_GRAPH_COUNT);
    if (persist_exists(PK_GRAPH_DATA)) persist_read_data(PK_GRAPH_DATA, s_graph_data, sizeof(s_graph_data));
    if (persist_exists(PK_GRAPH_ID)) s_graph_id = persist_read_int(PK_GRAPH_ID);
    if (persist_exists(PK_GRAPH_SCALE)) s_graph_scale = persist_read_int(PK_GRAPH_SCALE);
    if (out_activity) {
        if (persist_exists(PK_ACTIVITY_TYPE)) {
            *out_activity = (ActivityType)persist_read_int(PK_ACTIVITY_TYPE);
        } else {
            *out_activity = ACTIVITY_RUNNING;
        }
    }
#if defined(PBL_COLOR)
    if (out_color_idx) {
        if (persist_exists(PK_PERSONAL_COLOR)) {
            *out_color_idx = persist_read_int(PK_PERSONAL_COLOR);
        } else {
            *out_color_idx = 63; // GColorWhiteARGB8
        }
    }
#endif
}

void graph_data_clear(void) {
    s_graph_count = 0;
    s_graph_id = 0;
    s_graph_scale = 1;
    memset(s_graph_data, 0, sizeof(s_graph_data));
    graph_data_save_to_persist();
}

void graph_data_parse_hybrid(const char *input) {
    if (!input || input[0] == '\0') {
        s_graph_count = 0;
        return;
    }
    memset(s_graph_data, 0, sizeof(s_graph_data));
    const char *p = input;
    
    char type_str[8];
    app_extract_token(&p, type_str, sizeof(type_str));
    s_graph_id = atoi(type_str); 
    
    app_extract_token(&p, s_graph_y_label, sizeof(s_graph_y_label));
    app_extract_token(&p, s_graph_x_label, sizeof(s_graph_x_label));
    app_extract_token(&p, s_graph_max_label, sizeof(s_graph_max_label));
    app_extract_token(&p, s_graph_min_label, sizeof(s_graph_min_label));
    
    int idx = 0;
    while (*p && idx < MAX_GRAPH_DATA) {
        char val_str[16];
        app_extract_token(&p, val_str, sizeof(val_str));
        if (val_str[0] != '\0') {
            s_graph_data[idx++] = atoi(val_str);
        }
    }
    s_graph_count = idx;
}

/* 中段データパース（新旧両対応） */
void graph_data_parse_mid(const char *input) {
    s_mid_page_count = 0;
    if (!input || input[0] == '\0') return;
    const char *p = input;

    while (*p && s_mid_page_count < MAX_MID_PAGES) {
        MidPageData *page = &s_mid_pages[s_mid_page_count];
        char token1[16];
        app_extract_token(&p, token1, sizeof(token1));

        // 新形式: [ID],[NAME],[VAL],[UNIT] または 旧形式: [NAME],[VAL],[UNIT],[ICON_ID]
        if (app_is_numeric_string(token1)) {
            page->id = atoi(token1);
            app_extract_token(&p, page->name, sizeof(page->name));
            app_extract_token(&p, page->value, sizeof(page->value));
            app_extract_token(&p, page->unit, sizeof(page->unit));
            page->icon_id = 0;
        } else {
            // 旧形式互換
            page->id = s_mid_page_count + 1;
            snprintf(page->name, sizeof(page->name), "%s", token1);
            app_extract_token(&p, page->value, sizeof(page->value));
            app_extract_token(&p, page->unit, sizeof(page->unit));
            char icon_str[8];
            app_extract_token(&p, icon_str, sizeof(icon_str));
            page->icon_id = atoi(icon_str);
            if (page->name[0] == '\0' && strcmp(page->value, "DETAIL") == 0) {
                page->id = 0; // DETAIL画面はID:0
            }
        }
        s_mid_page_count++;
    }
    if (s_mid_page_count == 0 || s_current_mid_mode >= s_mid_page_count) {
        s_current_mid_mode = 0;
    }
}

/* 下段データパース（新旧両対応） */
void graph_data_parse_lower(const char *input) {
    s_lower_page_count = 0;
    if (!input || input[0] == '\0') return;
    const char *p = input;

    while (*p && s_lower_page_count < MAX_LOWER_PAGES) {
        LowerPageData *page = &s_lower_pages[s_lower_page_count];
        char token1[16];
        app_extract_token(&p, token1, sizeof(token1));

        if (app_is_numeric_string(token1)) {
            page->id = atoi(token1);
            app_extract_token(&p, page->name, sizeof(page->name));
            app_extract_token(&p, page->value, sizeof(page->value));
            app_extract_token(&p, page->unit, sizeof(page->unit));
            page->icon_id = 0;
        } else {
            page->id = s_lower_page_count + 1;
            snprintf(page->name, sizeof(page->name), "%s", token1);
            app_extract_token(&p, page->value, sizeof(page->value));
            app_extract_token(&p, page->unit, sizeof(page->unit));
            char icon_str[8];
            app_extract_token(&p, icon_str, sizeof(icon_str));
            page->icon_id = atoi(icon_str);
        }
        s_lower_page_count++;
    }
    if (s_lower_page_count == 0 || s_current_lower_mode >= s_lower_page_count) {
        s_current_lower_mode = 0;
    }
}

bool graph_data_is_detail_mode(uint8_t app_state) {
    if (app_state != 3) return true;
    if (s_mid_page_count == 0) return true;
    if (s_current_mid_mode < s_mid_page_count) {
        const MidPageData *page = &s_mid_pages[s_current_mid_mode];
        if (page->id == 0 || (page->name[0] == '\0' && strcmp(page->value, "DETAIL") == 0)) {
            return true;
        }
    }
    return false;
}

int graph_data_get_count(void) { return s_graph_count; }
int graph_data_get_id(void) { return s_graph_id; }
int graph_data_get_scale(void) { return s_graph_scale; }
int graph_data_get_point(int index) {
    if (index >= 0 && index < s_graph_count) return s_graph_data[index];
    return 0;
}

const char* graph_data_get_y_label(void) { return s_graph_y_label; }
const char* graph_data_get_x_label(void) { return s_graph_x_label; }
const char* graph_data_get_max_label(void) { return s_graph_max_label; }
const char* graph_data_get_min_label(void) { return s_graph_min_label; }

// 中段データアクセサ
int graph_data_get_mid_page_count(void) { return s_mid_page_count; }
int graph_data_get_current_mid_mode(void) { return s_current_mid_mode; }
void graph_data_set_current_mid_mode(int mode) { s_current_mid_mode = mode; }

bool graph_data_select_mid_by_id(int id) {
    for (int i = 0; i < s_mid_page_count; i++) {
        if (s_mid_pages[i].id == id) {
            s_current_mid_mode = i;
            return true;
        }
    }
    return false;
}

const MidPageData* graph_data_get_mid_page(int index) {
    if (index >= 0 && index < s_mid_page_count) return &s_mid_pages[index];
    return NULL;
}

const MidPageData* graph_data_get_current_mid_page(void) {
    if (s_mid_page_count > 0 && s_current_mid_mode < s_mid_page_count) {
        return &s_mid_pages[s_current_mid_mode];
    }
    return NULL;
}

// 下段データアクセサ
int graph_data_get_lower_page_count(void) { return s_lower_page_count; }
int graph_data_get_current_lower_mode(void) { return s_current_lower_mode; }
void graph_data_set_current_lower_mode(int mode) { s_current_lower_mode = mode; }

bool graph_data_select_lower_by_id(int id) {
    for (int i = 0; i < s_lower_page_count; i++) {
        if (s_lower_pages[i].id == id) {
            s_current_lower_mode = i;
            return true;
        }
    }
    return false;
}

const LowerPageData* graph_data_get_lower_page(int index) {
    if (index >= 0 && index < s_lower_page_count) return &s_lower_pages[index];
    return NULL;
}

const LowerPageData* graph_data_get_current_lower_page(void) {
    if (s_lower_page_count > 0 && s_current_lower_mode < s_lower_page_count) {
        return &s_lower_pages[s_current_lower_mode];
    }
    return NULL;
}

bool graph_data_is_lower_graph_mode(void) {
    // 下段リストが未受信の場合は従来のグラフ表示
    if (s_lower_page_count == 0) return true;
    const LowerPageData *page = graph_data_get_current_lower_page();
    if (!page) return true;
    // IDが100以上、または項目名が"GRAPH"の場合はグラフ表示
    return (page->id >= 100 || strcmp(page->name, "GRAPH") == 0);
}
