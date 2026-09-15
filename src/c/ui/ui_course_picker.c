#include "ui_course_picker.h"
#include <string.h>

static Layer *s_course_picker_layer = NULL;
static Layer *s_marquee_container_layer = NULL;
static TextLayer *s_marquee_text_layer = NULL;
static AppTimer *s_marquee_timer = NULL;

static bool s_is_course_picking = false;
static int s_selected_course_idx = 0;
static GColor s_current_bg, s_current_fg;

static CourseItem s_courses[MAX_COURSES];
static int s_course_count = 0;

static int s_marquee_offset_x = 0;
static int s_marquee_text_width = 0;
static int s_marquee_container_width = 0;
static int s_marquee_pause_counter = 0;

#define MARQUEE_SPEED_PX 1
#define MARQUEE_INTERVAL_MS 40
#define MARQUEE_PAUSE_TICKS 35 // 約 1.4秒ポーズ

static void stop_marquee(void) {
    if (s_marquee_timer) {
        app_timer_cancel(s_marquee_timer);
        s_marquee_timer = NULL;
    }
    s_marquee_offset_x = 0;
    s_marquee_pause_counter = 0;
}

static void marquee_timer_callback(void *context) {
    s_marquee_timer = NULL;
    if (!s_is_course_picking || !s_marquee_text_layer) return;

    if (s_marquee_pause_counter > 0) {
        s_marquee_pause_counter--;
        s_marquee_timer = app_timer_register(MARQUEE_INTERVAL_MS, marquee_timer_callback, NULL);
        return;
    }

    s_marquee_offset_x += MARQUEE_SPEED_PX;
    if (s_marquee_offset_x > s_marquee_text_width) {
        s_marquee_offset_x = 0;
        s_marquee_pause_counter = MARQUEE_PAUSE_TICKS;
    }

    layer_set_bounds(text_layer_get_layer(s_marquee_text_layer), GRect(s_marquee_offset_x, 0, s_marquee_text_width + 4, 30));

    s_marquee_timer = app_timer_register(MARQUEE_INTERVAL_MS, marquee_timer_callback, NULL);
}

static void update_marquee_content(void) {
    stop_marquee();
    if (!s_marquee_text_layer || s_course_count <= 0) return;

    const char *name = s_courses[s_selected_course_idx].name;
    text_layer_set_text(s_marquee_text_layer, name);

    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    GSize size = graphics_text_layout_get_content_size(
        name, font, GRect(0, 0, 600, 30), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft
    );
    s_marquee_text_width = size.w;

    layer_set_frame(text_layer_get_layer(s_marquee_text_layer), GRect(0, 0, s_marquee_text_width + 4, 30));
    layer_set_bounds(text_layer_get_layer(s_marquee_text_layer), GRect(0, 0, s_marquee_text_width + 4, 30));

    if (s_marquee_text_width > s_marquee_container_width) {
        // 幅を超える場合のみ、少し待ってからマーキーアニメーション開始
        s_marquee_pause_counter = MARQUEE_PAUSE_TICKS;
        s_marquee_timer = app_timer_register(MARQUEE_INTERVAL_MS, marquee_timer_callback, NULL);
    }
}

void ui_course_picker_init(void) {
    if (persist_exists(PK_COURSES_COUNT) && persist_exists(PK_COURSES_DATA)) {
        s_course_count = persist_read_int(PK_COURSES_COUNT);
        if (s_course_count < 0) s_course_count = 0;
        if (s_course_count > MAX_COURSES) s_course_count = MAX_COURSES;
        persist_read_data(PK_COURSES_DATA, s_courses, sizeof(s_courses));
    } else {
        // 初期状態: デフォルトコース
        s_course_count = 2;
        s_courses[0].id = 1;
        strncpy(s_courses[0].name, "COURSE A", COURSE_NAME_LEN - 1);
        s_courses[0].name[COURSE_NAME_LEN - 1] = '\0';
        s_courses[0].is_enabled = true;

        s_courses[1].id = 2;
        strncpy(s_courses[1].name, "COURSE B", COURSE_NAME_LEN - 1);
        s_courses[1].name[COURSE_NAME_LEN - 1] = '\0';
        s_courses[1].is_enabled = false;
        
        ui_course_picker_save();
    }
}

void ui_course_picker_save(void) {
    persist_write_int(PK_COURSES_COUNT, s_course_count);
    persist_write_data(PK_COURSES_DATA, s_courses, sizeof(s_courses));
}

int ui_course_picker_get_count(void) {
    return s_course_count;
}

const CourseItem* ui_course_picker_get_courses(void) {
    return s_courses;
}

void ui_course_picker_set_course(int idx, int id, const char *name, bool is_enabled) {
    if (idx < 0 || idx >= MAX_COURSES) return;
    s_courses[idx].id = id;
    if (name) {
        strncpy(s_courses[idx].name, name, COURSE_NAME_LEN - 1);
        s_courses[idx].name[COURSE_NAME_LEN - 1] = '\0';
    }
    s_courses[idx].is_enabled = is_enabled;
    if (idx >= s_course_count) {
        s_course_count = idx + 1;
    }
    ui_course_picker_save();
}

// Androidからの一括区切り文字列("1,NAME1|0,NAME2...")のパース＆一括保存
void ui_course_picker_parse_and_set(const char *data_str) {
    if (!data_str || !*data_str) return;

    const char *ptr = data_str;
    char token[32];
    int count = 0;

    while (*ptr && count < MAX_COURSES) {
        // 1. 有効フラグ (0 or 1)
        app_extract_token(&ptr, token, sizeof(token));
        bool enabled = (token[0] == '1');

        // 2. コース名
        app_extract_token(&ptr, token, sizeof(token));
        if (token[0] != '\0') {
            s_courses[count].id = count + 1;
            strncpy(s_courses[count].name, token, COURSE_NAME_LEN - 1);
            s_courses[count].name[COURSE_NAME_LEN - 1] = '\0';
            s_courses[count].is_enabled = enabled;
            count++;
        }
    }

    s_course_count = count;
    if (s_selected_course_idx >= s_course_count) {
        s_selected_course_idx = (s_course_count > 0) ? s_course_count - 1 : 0;
    }

    ui_course_picker_save();

    if (s_is_course_picking) {
        update_marquee_content();
        if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
    }
}

static void course_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_course_picking) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);

    int text_h = 14;
    graphics_context_set_text_color(ctx, s_current_fg);
    char title_buf[32];
    snprintf(title_buf, sizeof(title_buf), "COURSES (%d/%d)", s_course_count > 0 ? (s_selected_course_idx + 1) : 0, s_course_count);
    graphics_draw_text(ctx, title_buf, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 1, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);

    if (s_course_count <= 0) {
        graphics_draw_text(ctx, "NO COURSES", fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, 30, b.size.w, 24), 0, GTextAlignmentCenter, NULL);
        return;
    }

    int prev_idx = (s_selected_course_idx - 1 + s_course_count) % s_course_count;
    int next_idx = (s_selected_course_idx + 1) % s_course_count;

    char buf[32];

    // 1. 前の項目
    int prev_y = text_h + 3;
    graphics_context_set_text_color(ctx, s_current_fg);
    snprintf(buf, sizeof(buf), "%s %s", s_courses[prev_idx].is_enabled ? "[X]" : "[ ]", s_courses[prev_idx].name);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(4, prev_y, b.size.w - 8, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // 2. 選択中の項目（背景ハイライト矩形）
    int sel_box_y = prev_y + 21;
    int sel_box_h = 32;
    graphics_context_set_fill_color(ctx, s_current_fg);
    graphics_fill_rect(ctx, GRect(2, sel_box_y, b.size.w - 4, sel_box_h), 4, GCornersAll);

    // 【固定表示】チェックボックス領域（左端 26px）
    graphics_context_set_text_color(ctx, s_current_bg);
    const char *check_str = s_courses[s_selected_course_idx].is_enabled ? "[X]" : "[ ]";
    graphics_draw_text(ctx, check_str, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(6, sel_box_y + 2, 26, 28), 0, GTextAlignmentLeft, NULL);

    // 3. 次の項目
    int next_y = sel_box_y + sel_box_h + 3;
    graphics_context_set_text_color(ctx, s_current_fg);
    snprintf(buf, sizeof(buf), "%s %s", s_courses[next_idx].is_enabled ? "[X]" : "[ ]", s_courses[next_idx].name);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(4, next_y, b.size.w - 8, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

void ui_course_picker_create(Window *window, ActionBarLayer *action_bar, GColor main_bg, GColor main_fg) {
    if (s_course_picker_layer != NULL) return;
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    s_is_course_picking = true;

    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    s_course_picker_layer = layer_create(GRect(0, upper_h, w, h - upper_h));
#else
    int h3 = b.size.h / 3;
    int w = b.size.w - ACTION_BAR_WIDTH;
    s_course_picker_layer = layer_create(GRect(0, h3, w, b.size.h - h3));
#endif
    layer_set_update_proc(s_course_picker_layer, course_picker_update_proc);
    layer_add_child(wl, s_course_picker_layer);

    // マーキー用クリッピングコンテナの作成（左側チェックボックス 34px の右隣に配置）
    int text_h = 14;
    int prev_y = text_h + 3;
    int sel_box_y = prev_y + 21;
    int text_x = 34;
    s_marquee_container_width = w - text_x - 6;

    s_marquee_container_layer = layer_create(GRect(text_x, sel_box_y + 2, s_marquee_container_width, 28));
    layer_set_clips(s_marquee_container_layer, true); // はみ出た文字をクリッピング
    layer_add_child(s_course_picker_layer, s_marquee_container_layer);

    s_marquee_text_layer = text_layer_create(GRect(0, 0, s_marquee_container_width, 28));
    text_layer_set_font(s_marquee_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_color(s_marquee_text_layer, s_current_bg); // 反転色
    text_layer_set_background_color(s_marquee_text_layer, GColorClear);
    layer_add_child(s_marquee_container_layer, text_layer_get_layer(s_marquee_text_layer));

    update_marquee_content();
    layer_mark_dirty(s_course_picker_layer);
}

void ui_course_picker_destroy(void) {
    s_is_course_picking = false;
    stop_marquee();

    if (s_marquee_text_layer) {
        text_layer_destroy(s_marquee_text_layer);
        s_marquee_text_layer = NULL;
    }
    if (s_marquee_container_layer) {
        layer_destroy(s_marquee_container_layer);
        s_marquee_container_layer = NULL;
    }
    if (s_course_picker_layer) {
        layer_remove_from_parent(s_course_picker_layer);
        layer_destroy(s_course_picker_layer);
        s_course_picker_layer = NULL;
    }
}

bool ui_course_picker_is_active(void) {
    return s_is_course_picking;
}

void ui_course_picker_handle_up(void) {
    if (s_course_count <= 0) return;
    s_selected_course_idx = (s_selected_course_idx - 1 + s_course_count) % s_course_count;
    update_marquee_content();
    if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
}

void ui_course_picker_handle_down(void) {
    if (s_course_count <= 0) return;
    s_selected_course_idx = (s_selected_course_idx + 1) % s_course_count;
    update_marquee_content();
    if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
}

void ui_course_picker_handle_select(void) {
    if (s_course_count <= 0) return;
    s_courses[s_selected_course_idx].is_enabled = !s_courses[s_selected_course_idx].is_enabled;
    ui_course_picker_save();
    if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
}
