#include "ui_graph_layer.h"
#include "ui_marquee.h"
#include "../comm/graph_data.h"

void ui_graph_layer_update_proc(Layer *layer, GContext *ctx, uint8_t app_state, GColor fg_color) {
    if (app_state < 3 && app_state != 6) return;
    
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, fg_color);
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_context_set_text_color(ctx, fg_color);

    // 下段がテキスト項目モードの場合
    if (!graph_data_is_lower_graph_mode()) {
        const LowerPageData *page = graph_data_get_current_lower_page();
        if (page && page->name[0] != '\0') {
            int text_w = b.size.w - 10;
            int title_y = 2;
            int unit_y = b.size.h - 18;
            int val_y = (b.size.h / 2) - 16;

            graphics_draw_text(ctx, page->name, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(5, title_y, text_w, 16), 0, GTextAlignmentLeft, NULL);
            graphics_draw_text(ctx, page->value, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS), GRect(5, val_y, text_w, 36), 0, GTextAlignmentCenter, NULL);
            graphics_draw_text(ctx, page->unit, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(5, unit_y, text_w, 16), 0, GTextAlignmentRight, NULL);
        }
        return;
    }

    // グラフ描画
    int pl = 4;
    int tm = 16;
    int ah = b.size.h - tm;
    int dw = b.size.w - pl;

    int graph_id = graph_data_get_id();
    int graph_count = graph_data_get_count();

    int bw = dw / (graph_count > 0 ? graph_count : 1);
    if (bw < 2) bw = 2; 

    bool show_labels = !ui_marquee_is_active();
    if (show_labels) {
        graphics_draw_text(ctx, graph_data_get_y_label(), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(pl, 4, b.size.w, 16), 0, GTextAlignmentLeft, NULL);
        graphics_draw_text(ctx, graph_data_get_x_label(), fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, 4, b.size.w - 4, 16), 0, GTextAlignmentRight, NULL);
    }

    if (graph_count <= 0) return;

    int mah = (ah * 70) / 100;
    int prev_x = -1, prev_y = -1;

    if (graph_id == 0) { // PACE
        int p_min = 999999, p_max = -1;
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            if (pt > 0) {
                if (pt > p_max) p_max = pt;
                if (pt < p_min) p_min = pt;
            }
        }
        if (p_min == 999999) { p_min = 0; p_max = 1; }
        if (p_max == p_min) p_max = p_min + 1;

        int margin = (p_max - p_min) / 4;
        if (margin < 10) margin = 10;
        int plot_min = p_min - margin;
        if (plot_min < 0) plot_min = 0;
        int plot_max = p_max + margin;
        int plot_range = plot_max - plot_min;

        for (int i = 0; i < graph_count; i++) {
            int x = pl + i * bw + (bw / 2);
            int y;
            int pt = graph_data_get_point(i);
            if (pt <= 0) {
                y = b.size.h;
            } else {
                int val = pt;
                if (val > plot_max) val = plot_max;
                if (val < plot_min) val = plot_min;
                int bh = ((val - plot_min) * mah) / plot_range;
                y = b.size.h - mah + bh;
            }
            if (prev_x != -1) {
                graphics_context_set_stroke_width(ctx, 2);
                graphics_draw_line(ctx, GPoint(prev_x, prev_y), GPoint(x, y));
                graphics_context_set_stroke_width(ctx, 1);
            } else {
                graphics_fill_circle(ctx, GPoint(x, y), 2);
            }
            prev_x = x;
            prev_y = y;
        }
    } else if (graph_id == 1) { // SPEED
        int p_max = 1;
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            if (pt > p_max) p_max = pt;
        }
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            int bh = (pt * mah) / p_max;
            if (bh < 0) bh = 0;
            int bar_w = bw - 1;
            if (bar_w < 1) bar_w = 1;
            graphics_fill_rect(ctx, GRect(pl + i * bw, b.size.h - bh, bar_w, bh), 0, GCornerNone);
        }
    } else if (graph_id == 2) { // HR
        int p_min = 999999, p_max = -1;
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            if (pt > 0) {
                if (pt > p_max) p_max = pt;
                if (pt < p_min) p_min = pt;
            }
        }
        if (p_min == 999999) { p_min = 60; p_max = 120; }
        if (p_max == p_min) p_max = p_min + 10;

        int plot_max = p_max + 10;
        int plot_min = p_min - 20;
        if (plot_min < 0) plot_min = 0;
        int plot_range = plot_max - plot_min;

        for (int i = 0; i < graph_count; i++) {
            int val = graph_data_get_point(i);
            if (val <= 0) continue; 

            if (val > plot_max) val = plot_max;
            if (val < plot_min) val = plot_min;
            
            int bh = ((val - plot_min) * mah) / plot_range;
            if (bh < 2) bh = 2; 
            
            int bar_w = bw - 1;
            if (bar_w < 1) bar_w = 1;
            graphics_fill_rect(ctx, GRect(pl + i * bw, b.size.h - bh, bar_w, bh), 0, GCornerNone);
        }
    } else if (graph_id == 3) { // ELEVATION
        int p_min = 999999, p_max = -999999;
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            if (pt > p_max) p_max = pt;
            if (pt < p_min) p_min = pt;
        }
        if (p_min == 999999) { p_min = 0; p_max = 100; }
        if (p_max == p_min) { p_max = p_min + 10; p_min -= 10; }
        
        int margin = (p_max - p_min) / 4;
        if (margin < 5) margin = 5;
        int plot_min = p_min - margin;
        int plot_max = p_max + margin;
        int plot_range = plot_max - plot_min;

        for (int i = 0; i < graph_count; i++) {
            int x = pl + i * bw + (bw / 2);
            int val = graph_data_get_point(i);
            
            if (val > plot_max) val = plot_max;
            if (val < plot_min) val = plot_min;
            int bh = ((val - plot_min) * mah) / plot_range;
            int y = b.size.h - bh; 

            if (prev_x != -1) {
                graphics_context_set_stroke_width(ctx, 2);
                graphics_draw_line(ctx, GPoint(prev_x, prev_y), GPoint(x, y));
                graphics_context_set_stroke_width(ctx, 1);
            } else {
                graphics_fill_circle(ctx, GPoint(x, y), 2);
            }
            prev_x = x;
            prev_y = y;
        }

        if (show_labels) {
            char max_str[16], min_str[16];
            snprintf(max_str, 16, "%dm", p_max);
            snprintf(min_str, 16, "%dm", p_min);
            
            int lbl_w = 42;
            int lbl_x = b.size.w - lbl_w;
            int lbl_h = 16;
            graphics_draw_text(ctx, max_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - mah - 2, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
            graphics_draw_text(ctx, min_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - 16, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
        }
    } else if (graph_id == 4) { // CADENCE
        int p_min = 999999, p_max = -1;
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            if (pt > 0) {
                if (pt > p_max) p_max = pt;
                if (pt < p_min) p_min = pt;
            }
        }
        if (p_min == 999999) { p_min = 60; p_max = 180; }
        if (p_max == p_min) p_max = p_min + 1;
        
        int margin = (p_max - p_min) / 4;
        if (margin < 5) margin = 5;
        int plot_min = p_min - margin;
        if (plot_min < 0) plot_min = 0;
        int plot_max = p_max + margin;
        int plot_range = plot_max - plot_min;

        for (int i = 0; i < graph_count; i++) {
            int x = pl + i * bw + (bw / 2);
            int y;
            int pt = graph_data_get_point(i);
            if (pt <= 0) {
                y = b.size.h;
            } else {
                int val = pt;
                if (val > plot_max) val = plot_max;
                if (val < plot_min) val = plot_min;
                int bh = ((val - plot_min) * mah) / plot_range;
                y = b.size.h - bh; 
            }
            if (prev_x != -1) {
                graphics_context_set_stroke_width(ctx, 2);
                graphics_draw_line(ctx, GPoint(prev_x, prev_y), GPoint(x, y));
                graphics_context_set_stroke_width(ctx, 1);
            } else {
                graphics_fill_circle(ctx, GPoint(x, y), 2);
            }
            prev_x = x;
            prev_y = y;
        }

        if (show_labels) {
            char max_str[16], min_str[16];
            snprintf(max_str, 16, "%d", p_max);
            snprintf(min_str, 16, "%d", p_min);
            
            int lbl_w = 28;
            int lbl_x = b.size.w - lbl_w;
            int lbl_h = 16;
            graphics_draw_text(ctx, max_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - mah - 2, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
            graphics_draw_text(ctx, min_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - 16, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
        }
    } else if (graph_id == 5) { // CALORIES
        int p_max = 1;
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            if (pt > p_max) p_max = pt;
        }
        for (int i = 0; i < graph_count; i++) {
            int pt = graph_data_get_point(i);
            int bh = (pt * mah) / p_max;
            if (bh < 0) bh = 0;
            int bar_w = bw - 1;
            if (bar_w < 1) bar_w = 1; 
            graphics_fill_rect(ctx, GRect(pl + i * bw, b.size.h - bh, bar_w, bh), 0, GCornerNone);
        }
        if (show_labels) {
            int lbl_w = 48;
            int lbl_x = b.size.w - lbl_w;
            int lbl_h = 16;
            graphics_draw_text(ctx, graph_data_get_max_label(), fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - mah - 2, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
        }
    }
}

void ui_mid_bg_layer_update_proc(Layer *layer, GContext *ctx, uint8_t app_state, GColor main_bg, GColor main_fg, bool is_paused, int current_hr) {
    GRect b = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, main_fg);
    graphics_context_set_fill_color(ctx, main_fg);
    graphics_context_set_text_color(ctx, main_fg);

    int wt = b.size.w;
    int h = b.size.h;

    int mx = 0, upper_h = 0, mid_h = 0, r1y = 0, r2y = 0, lx = 0, rx = 0;
    int active_w = wt - ACTION_BAR_WIDTH;

#if defined(PBL_PLATFORM_APLITE) || defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT)
    mx = (wt <= 144) ? 5 : 10;
    upper_h = h / 3;
    mid_h = h / 3;
    r1y = upper_h + 17;
    r2y = upper_h + 45;
    lx = mx + 6;
    rx = ((wt - ACTION_BAR_WIDTH) / 2) + 10;
#elif defined(PBL_PLATFORM_CHALK)
    mid_h = 45;
    upper_h = (h - mid_h) / 2;
    mx = 20;
    int base_y = upper_h + (mid_h - 18 * 2) / 2 + 9;
    r1y = base_y;
    r2y = base_y + 18;
    lx = mx - 4;
    rx = ((wt - ACTION_BAR_WIDTH) / 2) + 16;
#elif defined(PBL_PLATFORM_EMERY)
    mx = 10;
    upper_h = h / 3;
    mid_h = h / 3;
    r1y = upper_h + 22;
    r2y = upper_h + 50;
    lx = mx + 6;
    rx = ((wt - ACTION_BAR_WIDTH) / 2) + 16;
#else // PR2 / GABBRO
    mid_h = 65;
    upper_h = (h - mid_h) / 2;
    mx = 30;
    int base_y = upper_h + (mid_h - 28 * 2) / 2 + 12;
    r1y = base_y;
    r2y = base_y + 28;
    lx = mx + 10;
    rx = ((wt - ACTION_BAR_WIDTH) / 2) + 22;
#endif

    int fill_w = wt;
    bool is_active = (app_state == 3);

    if (is_active) {
        graphics_context_set_fill_color(ctx, main_fg);
        graphics_fill_rect(ctx, GRect(0, upper_h, fill_w, mid_h), 0, GCornerNone);
        graphics_context_set_text_color(ctx, main_bg);
        graphics_context_set_stroke_color(ctx, main_bg);
    } else {
        graphics_context_set_text_color(ctx, main_fg);
        graphics_context_set_stroke_color(ctx, main_fg);
    }

    if (!graph_data_is_detail_mode(app_state)) {
        const MidPageData *page = graph_data_get_current_mid_page();
        if (page) {
            int text_w = is_active ? (fill_w - 10) : (active_w - 10);
            int title_y = upper_h;
            int unit_y = upper_h + mid_h - 20;

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
            int val_y = upper_h + (mid_h / 2) - 34; 
            graphics_draw_text(ctx, page->value, fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM), GRect(0, val_y, active_w, 70), 0, GTextAlignmentCenter, NULL);
#else
            int val_y;
#if defined(PBL_PLATFORM_CHALK)
            val_y = upper_h + (mid_h / 2) - 14; 
#else
            val_y = upper_h + (mid_h / 2) - 20; 
#endif
            graphics_draw_text(ctx, page->value, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS), GRect(5, val_y, active_w - 10, 48), 0, GTextAlignmentCenter, NULL);
#endif
            graphics_draw_text(ctx, page->name, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(5, title_y, text_w, 20), 0, GTextAlignmentLeft, NULL);
            graphics_draw_text(ctx, page->unit, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(5, unit_y, text_w, 20), 0, GTextAlignmentRight, NULL);
        }
        return;
    }

    if (!is_active) {
        int line_end_w = wt - ACTION_BAR_WIDTH - mx;
        graphics_draw_line(ctx, GPoint(mx, upper_h), GPoint(line_end_w, upper_h));
        graphics_draw_line(ctx, GPoint(mx, upper_h + mid_h), GPoint(line_end_w, upper_h + mid_h));
    }

    graphics_context_set_fill_color(ctx, is_active ? main_bg : main_fg);
    graphics_fill_circle(ctx, GPoint(lx, r1y), 3);
    graphics_draw_line(ctx, GPoint(lx - 2, r1y + 2), GPoint(lx, r1y + 6));
    graphics_draw_line(ctx, GPoint(lx + 2, r1y + 2), GPoint(lx, r1y + 6));
    
    graphics_fill_rect(ctx, GRect(rx - 1, r1y, 3, 5), 1, GCornerNone);
    graphics_fill_circle(ctx, GPoint(rx + 2, r1y - 2), 1);
    graphics_fill_circle(ctx, GPoint(rx, r1y - 3), 1);

#if defined(PBL_HEALTH)
    graphics_fill_circle(ctx, GPoint(lx - 2, r2y), 2);
    graphics_fill_circle(ctx, GPoint(lx + 2, r2y), 2);
    graphics_draw_line(ctx, GPoint(lx - 4, r2y + 1), GPoint(lx, r2y + 5));
    graphics_draw_line(ctx, GPoint(lx + 4, r2y + 1), GPoint(lx, r2y + 5));
#endif

    graphics_draw_circle(ctx, GPoint(rx, r2y), 4);
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx, r2y - 2));
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx + 2, r2y));
}
