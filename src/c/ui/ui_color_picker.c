#include "ui_color_picker.h"

#if defined(PBL_COLOR)

static Layer *s_color_picker_layer = NULL;
static bool s_is_color_picking = false;
static int s_selected_color_idx = 24; // GColorIslamicGreenARGB8
static bool s_preview_is_running = false; 
static uint8_t s_personal_color_argb = GColorIslamicGreenARGB8; 
static GColor s_current_bg, s_current_fg;

static void color_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_color_picking) return;
    
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 18;
    graphics_context_set_text_color(ctx, s_current_fg);
    graphics_draw_text(ctx, "PICK YOUR COLOR", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);
    
    int grid = 8;
    int available_h = b.size.h - text_h;
    int box = (b.size.w > available_h ? available_h : b.size.w) / grid;
    if (box > 14) box = 14;
    
    int ox = (b.size.w - (grid * box)) / 2;
    int oy = text_h + (available_h - (grid * box)) / 2;
    
    for (int i = 0; i < 64; i++) {
        GRect r = GRect(ox + (i % grid) * box, oy + (i / grid) * box, box, box);
        GColor c = (GColor){.argb = i + 0b11000000};
        
        graphics_context_set_fill_color(ctx, c);
        graphics_fill_rect(ctx, r, 0, GCornerNone);
        
        if (i == s_selected_color_idx) {
            graphics_context_set_stroke_color(ctx, gcolor_legible_over(c));
            graphics_context_set_stroke_width(ctx, 2);
            graphics_draw_rect(ctx, r);
            graphics_context_set_stroke_width(ctx, 1);
            
            graphics_context_set_stroke_color(ctx, c);
            graphics_draw_rect(ctx, GRect(r.origin.x + 2, r.origin.y + 2, r.size.w - 4, r.size.h - 4));
        }
    }
}

void ui_color_picker_create(Window *window, ActionBarLayer *action_bar, GColor main_bg, GColor main_fg) {
    if (s_color_picker_layer != NULL) return;
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    s_is_color_picking = true;
    s_preview_is_running = false;
    
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    s_color_picker_layer = layer_create(GRect(0, upper_h, w, h - upper_h));
#else
    int h3 = b.size.h / 3;
    int w = b.size.w - ACTION_BAR_WIDTH;
    s_color_picker_layer = layer_create(GRect(0, h3, w, b.size.h - h3));
#endif
    layer_set_update_proc(s_color_picker_layer, color_picker_update_proc);
    if (action_bar) {
        layer_insert_below_sibling(s_color_picker_layer, action_bar_layer_get_layer(action_bar));
    } else {
        layer_add_child(wl, s_color_picker_layer);
    }
}

void ui_color_picker_destroy(void) {
    s_is_color_picking = false;
    if (s_color_picker_layer) {
        layer_destroy(s_color_picker_layer);
        s_color_picker_layer = NULL;
    }
}

bool ui_color_picker_is_active(void) { return s_is_color_picking; }

void ui_color_picker_handle_down(void) {
    s_selected_color_idx = (s_selected_color_idx + 1) % 64;
    s_personal_color_argb = (GColor){.argb = s_selected_color_idx + 0b11000000}.argb;
    if (s_color_picker_layer) layer_mark_dirty(s_color_picker_layer);
}

int ui_color_picker_get_selected_idx(void) { return s_selected_color_idx; }

void ui_color_picker_set_selected_idx(int idx) {
    s_selected_color_idx = idx;
    s_personal_color_argb = (GColor){.argb = idx + 0b11000000}.argb;
    if (s_color_picker_layer) layer_mark_dirty(s_color_picker_layer);
}

uint8_t ui_color_picker_get_personal_color_argb(void) { return s_personal_color_argb; }

void ui_color_picker_toggle_preview(void) {
    s_preview_is_running = !s_preview_is_running;
}

bool ui_color_picker_get_preview_is_running(void) { return s_preview_is_running; }

void ui_color_picker_update_colors(GColor main_bg, GColor main_fg) {
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    if (s_color_picker_layer) layer_mark_dirty(s_color_picker_layer);
}

#endif
