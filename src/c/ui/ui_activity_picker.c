#include "../app_state.h"
#include "ui_activity_picker.h"

static Layer *s_activity_picker_layer = NULL;
static bool s_is_activity_picking = false;
static int s_preview_activity_idx = 0;
static GColor s_current_bg, s_current_fg;

static ActivityItem s_activities[MAX_ACTIVITIES];
static int s_activity_count = 0;
static int s_activities_version = 0;

static const char* const DEFAULT_NAMES[] = {
    "RUNNING", "WALKING", "CYCLING", "HIKING", 
    "KAYAKING", "ROWING", "OTHER"
};
#define DEFAULT_COUNT 7

static void set_default_activities(void) {
    s_activity_count = DEFAULT_COUNT;
    s_activities_version = 0;
    for (int i = 0; i < DEFAULT_COUNT && i < MAX_ACTIVITIES; i++) {
        s_activities[i].id = (uint8_t)i;
        strncpy(s_activities[i].name, DEFAULT_NAMES[i], ACTIVITY_NAME_LEN - 1);
        s_activities[i].name[ACTIVITY_NAME_LEN - 1] = '\0';
    }
}

void ui_activity_picker_init(void) {
    if (persist_exists(PK_ACTIVITIES_COUNT) && persist_exists(PK_ACTIVITIES_DATA)) {
        s_activity_count = persist_read_int(PK_ACTIVITIES_COUNT);
        if (s_activity_count <= 0 || s_activity_count > MAX_ACTIVITIES) {
            set_default_activities();
            ui_activity_picker_save();
        } else {
            int first_count = (s_activity_count > ACTIVITIES_PERSIST_CHUNK) ? ACTIVITIES_PERSIST_CHUNK : s_activity_count;
            persist_read_data(PK_ACTIVITIES_DATA, &s_activities[0], sizeof(ActivityItem) * first_count);

            if (s_activity_count > ACTIVITIES_PERSIST_CHUNK) {
                if (persist_exists(PK_ACTIVITIES_DATA_2)) {
                    int second_count = s_activity_count - ACTIVITIES_PERSIST_CHUNK;
                    persist_read_data(PK_ACTIVITIES_DATA_2, &s_activities[ACTIVITIES_PERSIST_CHUNK], sizeof(ActivityItem) * second_count);
                } else {
                    set_default_activities();
                    ui_activity_picker_save();
                    return;
                }
            }
            s_activities_version = persist_exists(PK_ACTIVITIES_VERSION) ? persist_read_int(PK_ACTIVITIES_VERSION) : 0;
        }
    } else {
        set_default_activities();
        ui_activity_picker_save();
    }
}

void ui_activity_picker_save(void) {
    persist_write_int(PK_ACTIVITIES_COUNT, s_activity_count);
    int first_count = (s_activity_count > ACTIVITIES_PERSIST_CHUNK) ? ACTIVITIES_PERSIST_CHUNK : s_activity_count;
    if (first_count > 0) {
        persist_write_data(PK_ACTIVITIES_DATA, &s_activities[0], sizeof(ActivityItem) * first_count);
    }
    if (s_activity_count > ACTIVITIES_PERSIST_CHUNK) {
        int second_count = s_activity_count - ACTIVITIES_PERSIST_CHUNK;
        persist_write_data(PK_ACTIVITIES_DATA_2, &s_activities[ACTIVITIES_PERSIST_CHUNK], sizeof(ActivityItem) * second_count);
    } else if (persist_exists(PK_ACTIVITIES_DATA_2)) {
        persist_delete(PK_ACTIVITIES_DATA_2);
    }
    persist_write_int(PK_ACTIVITIES_VERSION, s_activities_version);
}

int ui_activity_picker_get_count(void) {
    if (s_activity_count == 0) ui_activity_picker_init();
    return s_activity_count;
}

const ActivityItem* ui_activity_picker_get_activities(void) {
    if (s_activity_count == 0) ui_activity_picker_init();
    return s_activities;
}

bool ui_activity_picker_is_valid_id(uint8_t id) {
    if (s_activity_count == 0) ui_activity_picker_init();
    for (int i = 0; i < s_activity_count; i++) {
        if (s_activities[i].id == id) return true;
    }
    return false;
}

const char* ui_activity_picker_get_name_by_id(uint8_t id) {
    if (s_activity_count == 0) ui_activity_picker_init();
    for (int i = 0; i < s_activity_count; i++) {
        if (s_activities[i].id == id) return s_activities[i].name;
    }
    return "UNKNOWN";
}

void ui_activity_picker_parse_and_set(const char *data_str) {
    if (!data_str || *data_str == '\0') return;

    const char *ptr = data_str;
    char token[32];
    int version = 0;

    // バージョンヘッダーチェック ("v:1|" または "v1|")
    if (*ptr == 'v' || *ptr == 'V') {
        app_extract_token(&ptr, token, sizeof(token));
        const char *v_num = token + 1;
        if (*v_num == ':') v_num++;
        version = atoi(v_num);
    }

    ActivityItem temp_items[MAX_ACTIVITIES];
    int count = 0;

    while (*ptr && count < MAX_ACTIVITIES) {
        // 1. ID
        app_extract_token(&ptr, token, sizeof(token));
        if (token[0] == '\0') break;
        int id_val = atoi(token);

        // 2. NAME
        app_extract_token(&ptr, token, sizeof(token));
        if (token[0] == '\0') break;

        temp_items[count].id = (uint8_t)id_val;
        strncpy(temp_items[count].name, token, ACTIVITY_NAME_LEN - 1);
        temp_items[count].name[ACTIVITY_NAME_LEN - 1] = '\0';
        count++;
    }

    if (count > 0) {
        s_activity_count = count;
        s_activities_version = version;
        memcpy(s_activities, temp_items, sizeof(ActivityItem) * count);
        ui_activity_picker_save();

        if (s_preview_activity_idx >= s_activity_count) {
            s_preview_activity_idx = 0;
        }

        if (s_is_activity_picking && s_activity_picker_layer) {
            layer_mark_dirty(s_activity_picker_layer);
        }
    }
}

static void activity_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_activity_picking || s_activity_count <= 0) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 14;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, "ACTIVITY TYPE", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 1, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);
    
    int prev_idx = (s_preview_activity_idx - 1 + s_activity_count) % s_activity_count;
    int next_idx = (s_preview_activity_idx + 1) % s_activity_count;
    
    // 1. 前の項目（フォントサイズ GOTHIC_18）
    int prev_y = text_h + 3;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, s_activities[prev_idx].name, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, prev_y, b.size.w, 20), 0, GTextAlignmentCenter, NULL);
    
    // 2. 選択中の項目（最大級フォント GOTHIC_28_BOLD でド迫力表示）
    int sel_box_y = prev_y + 21;
    int sel_box_h = 38;
    graphics_context_set_fill_color(ctx, s_current_fg);
    graphics_fill_rect(ctx, GRect(2, sel_box_y, b.size.w - 4, sel_box_h), 5, GCornersAll);
    
    graphics_context_set_text_color(ctx, s_current_bg);
    graphics_draw_text(ctx, s_activities[s_preview_activity_idx].name, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GRect(0, sel_box_y + 2, b.size.w, 34), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    
    // 3. 次の項目（フォントサイズ GOTHIC_18）
    int next_y = sel_box_y + sel_box_h + 3;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, s_activities[next_idx].name, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, next_y, b.size.w, 20), 0, GTextAlignmentCenter, NULL);
}

void ui_activity_picker_create(Window *window, ActionBarLayer *action_bar, ActivityType initial_type, GColor main_bg, GColor main_fg) {
    if (s_activity_picker_layer != NULL) return;
    if (s_activity_count == 0) {
        ui_activity_picker_init();
    }
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    s_is_activity_picking = true;
    ui_activity_picker_set_selected_id((uint8_t)initial_type);
    
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    s_activity_picker_layer = layer_create(GRect(0, upper_h, w, h - upper_h));
#else
    int h3 = b.size.h / 3;
    int w = b.size.w - ACTION_BAR_WIDTH;
    s_activity_picker_layer = layer_create(GRect(0, h3, w, b.size.h - h3));
#endif
    layer_set_update_proc(s_activity_picker_layer, activity_picker_update_proc);
    if (action_bar) layer_insert_below_sibling(s_activity_picker_layer, action_bar_layer_get_layer(action_bar));
    else layer_add_child(wl, s_activity_picker_layer);
}

void ui_activity_picker_destroy(void) {
    s_is_activity_picking = false;
    if (s_activity_picker_layer) {
        layer_destroy(s_activity_picker_layer);
        s_activity_picker_layer = NULL;
    }
}

bool ui_activity_picker_is_active(void) { return s_is_activity_picking; }

void ui_activity_picker_handle_up(void) {
    if (s_activity_count <= 0) return;
    s_preview_activity_idx = (s_preview_activity_idx - 1 + s_activity_count) % s_activity_count;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}

void ui_activity_picker_handle_down(void) {
    if (s_activity_count <= 0) return;
    s_preview_activity_idx = (s_preview_activity_idx + 1) % s_activity_count;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}

uint8_t ui_activity_picker_get_selected_id(void) {
    if (s_preview_activity_idx >= 0 && s_preview_activity_idx < s_activity_count) {
        return s_activities[s_preview_activity_idx].id;
    }
    return 0;
}

void ui_activity_picker_set_selected_id(uint8_t id) {
    if (s_activity_count == 0) ui_activity_picker_init();
    for (int i = 0; i < s_activity_count; i++) {
        if (s_activities[i].id == id) {
            s_preview_activity_idx = i;
            if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
            return;
        }
    }
    s_preview_activity_idx = 0;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}

ActivityType ui_activity_picker_get_preview_activity(void) {
    return (ActivityType)ui_activity_picker_get_selected_id();
}

void ui_activity_picker_set_preview_activity(ActivityType type) {
    ui_activity_picker_set_selected_id((uint8_t)type);
}

void ui_activity_picker_update_colors(GColor main_bg, GColor main_fg) {
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}
