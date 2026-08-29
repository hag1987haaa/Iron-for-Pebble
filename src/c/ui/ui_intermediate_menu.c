#include "ui_intermediate_menu.h"
#include "../app_state.h"

static Layer *s_intermediate_layer = NULL;
static bool s_is_intermediate_menu = false;
static int s_intermediate_idx = 0; 
static GColor s_current_bg, s_current_fg;

#if defined(PBL_COLOR)
#define INTERMEDIATE_MENU_COUNT 2
static const char* const s_menu_labels[INTERMEDIATE_MENU_COUNT] = {
    "Activity Type",
    "Custom Color"
};
#else
#define INTERMEDIATE_MENU_COUNT 1
static const char* const s_menu_labels[INTERMEDIATE_MENU_COUNT] = {
    "Activity Type"
};
#endif

static void intermediate_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_intermediate_menu) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 16;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, "SETTINGS", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);
    
    int item_h = 24;
    int spacing = 4;
    int oy = text_h + 4;
    
    for (int i = 0; i < INTERMEDIATE_MENU_COUNT; i++) {
        if (s_intermediate_idx == i) {
            graphics_context_set_fill_color(ctx, s_current_fg);
            graphics_fill_rect(ctx, GRect(5, oy, b.size.w - 10, item_h + 2), 4, GCornersAll);
            graphics_context_set_text_color(ctx, s_current_bg);
        } else {
            graphics_context_set_text_color(ctx, s_current_fg);
        }
        graphics_draw_text(ctx, s_menu_labels[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0, oy + 1, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
        oy += item_h + spacing;
    }

    // 一番下段中央にバージョン表示
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, APP_VERSION_STR, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, b.size.h - 16, b.size.w, 14), 0, GTextAlignmentCenter, NULL);
}

void ui_intermediate_menu_create(Window *window, ActionBarLayer *action_bar, GColor main_bg, GColor main_fg) {
    if (s_intermediate_layer != NULL) return;
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    s_is_intermediate_menu = true;
    s_intermediate_idx = 0;
    
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    s_intermediate_layer = layer_create(GRect(0, upper_h, w, h - upper_h));
#else
    int h3 = b.size.h / 3;
    int w = b.size.w - ACTION_BAR_WIDTH;
    s_intermediate_layer = layer_create(GRect(0, h3, w, b.size.h - h3));
#endif
    layer_set_update_proc(s_intermediate_layer, intermediate_update_proc);
    if (action_bar) layer_insert_below_sibling(s_intermediate_layer, action_bar_layer_get_layer(action_bar));
    else layer_add_child(wl, s_intermediate_layer);
}

void ui_intermediate_menu_destroy(void) {
    s_is_intermediate_menu = false;
    if (s_intermediate_layer) {
        layer_destroy(s_intermediate_layer);
        s_intermediate_layer = NULL;
    }
}

bool ui_intermediate_menu_is_active(void) { return s_is_intermediate_menu; }

void ui_intermediate_menu_handle_up(void) {
    s_intermediate_idx = (s_intermediate_idx - 1 + INTERMEDIATE_MENU_COUNT) % INTERMEDIATE_MENU_COUNT;
    if (s_intermediate_layer) layer_mark_dirty(s_intermediate_layer);
}

void ui_intermediate_menu_handle_down(void) {
    s_intermediate_idx = (s_intermediate_idx + 1) % INTERMEDIATE_MENU_COUNT;
    if (s_intermediate_layer) layer_mark_dirty(s_intermediate_layer);
}

int ui_intermediate_menu_get_selected_idx(void) { return s_intermediate_idx; }

void ui_intermediate_menu_set_selected_idx(int idx) {
    s_intermediate_idx = (idx % INTERMEDIATE_MENU_COUNT + INTERMEDIATE_MENU_COUNT) % INTERMEDIATE_MENU_COUNT;
    if (s_intermediate_layer) layer_mark_dirty(s_intermediate_layer);
}

void ui_intermediate_menu_update_colors(GColor main_bg, GColor main_fg) {
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    if (s_intermediate_layer) layer_mark_dirty(s_intermediate_layer);
}
