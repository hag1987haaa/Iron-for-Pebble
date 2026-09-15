#include "ui_course_picker.h"
#include <string.h>

static Layer *s_course_picker_layer = NULL;
static bool s_is_course_picking = false;
static int s_selected_course_idx = 0;
static GColor s_current_bg, s_current_fg;

static CourseItem s_courses[MAX_COURSES];
static int s_course_count = 0;

void ui_course_picker_init(void) {
    if (persist_exists(PK_COURSES_COUNT) && persist_exists(PK_COURSES_DATA)) {
        s_course_count = persist_read_int(PK_COURSES_COUNT);
        if (s_course_count < 0) s_course_count = 0;
        if (s_course_count > MAX_COURSES) s_course_count = MAX_COURSES;
        persist_read_data(PK_COURSES_DATA, s_courses, sizeof(s_courses));
    } else {
        // 初期状態: デフォルトのプレースホルダコース
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

static void course_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_course_picking) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);

    int text_h = 14;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, "COURSES", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 1, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);

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

    // 2. 選択中の項目（強調ボックス表示）
    int sel_box_y = prev_y + 21;
    int sel_box_h = 32;
    graphics_context_set_fill_color(ctx, s_current_fg);
    graphics_fill_rect(ctx, GRect(2, sel_box_y, b.size.w - 4, sel_box_h), 4, GCornersAll);

    graphics_context_set_text_color(ctx, s_current_bg);
    snprintf(buf, sizeof(buf), "%s %s", s_courses[s_selected_course_idx].is_enabled ? "[X]" : "[ ]", s_courses[s_selected_course_idx].name);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(6, sel_box_y + 2, b.size.w - 12, 28), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

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
    layer_mark_dirty(s_course_picker_layer);
}

void ui_course_picker_destroy(void) {
    s_is_course_picking = false;
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
    if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
}

void ui_course_picker_handle_down(void) {
    if (s_course_count <= 0) return;
    s_selected_course_idx = (s_selected_course_idx + 1) % s_course_count;
    if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
}

void ui_course_picker_handle_select(void) {
    if (s_course_count <= 0) return;
    s_courses[s_selected_course_idx].is_enabled = !s_courses[s_selected_course_idx].is_enabled;
    ui_course_picker_save();
    if (s_course_picker_layer) layer_mark_dirty(s_course_picker_layer);
}
