#include "../app_state.h"
#include "ui_activity_picker.h"

static Layer *s_activity_picker_layer = NULL;
static bool s_is_activity_picking = false;
static int s_preview_activity_idx = 0;
static GColor s_current_bg, s_current_fg;

static void activity_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_activity_picking) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 18;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, "ACTIVITY TYPE", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);
    
    int item_h = 24;
    int oy = text_h + 6;
    int prev_idx = (s_preview_activity_idx - 1 + ACTIVITY_COUNT) % ACTIVITY_COUNT;
    int next_idx = (s_preview_activity_idx + 1) % ACTIVITY_COUNT;
    
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, ACTIVITY_NAMES[prev_idx], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, oy, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
    oy += item_h + 4;
    
    graphics_context_set_fill_color(ctx, s_current_fg);
    graphics_fill_rect(ctx, GRect(5, oy, b.size.w - 10, item_h + 4), 4, GCornersAll);
    graphics_context_set_text_color(ctx, s_current_bg);
    graphics_draw_text(ctx, ACTIVITY_NAMES[s_preview_activity_idx], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0, oy + 2, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
    oy += item_h + 8;
    
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, ACTIVITY_NAMES[next_idx], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, oy, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);

    // 一番下段中央にバージョン表示
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, APP_VERSION_STR, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, b.size.h - 15, b.size.w, 14), 0, GTextAlignmentCenter, NULL);
}

void ui_activity_picker_create(Window *window, ActionBarLayer *action_bar, ActivityType initial_type, GColor main_bg, GColor main_fg) {
    if (s_activity_picker_layer != NULL) return;
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    s_is_activity_picking = true;
    s_preview_activity_idx = (int)initial_type;
    
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
    s_preview_activity_idx = (s_preview_activity_idx - 1 + ACTIVITY_COUNT) % ACTIVITY_COUNT;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}

void ui_activity_picker_handle_down(void) {
    s_preview_activity_idx = (s_preview_activity_idx + 1) % ACTIVITY_COUNT;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}

ActivityType ui_activity_picker_get_preview_activity(void) {
    return (ActivityType)s_preview_activity_idx;
}

void ui_activity_picker_set_preview_activity(ActivityType type) {
    s_preview_activity_idx = (int)type;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}

void ui_activity_picker_update_colors(GColor main_bg, GColor main_fg) {
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
}
