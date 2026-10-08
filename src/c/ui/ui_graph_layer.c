#include "ui_color_picker.h"
#include "ui_graph_layer.h"
#include "ui_marquee.h"
#include "../comm/graph_data.h"

// 汎用メトリック描画部品（中段・下段共通）
void ui_metric_render_page(GContext *ctx, GRect bounds, const MetricPageData *page, bool is_inverted, GColor main_bg, GColor main_fg, int screen_width) {
    if (!page || page->name[0] == 0) return;

    int active_w = bounds.size.w;
    int h = bounds.size.h;
    int y0 = bounds.origin.y;
    int wt = (screen_width > 0) ? screen_width : (active_w + ACTION_BAR_WIDTH);

    GColor title_color;
    GColor val_color;
    GColor unit_color;

    if (is_inverted) {
        // 中段反転ブロック描画（背景を main_fg で全幅塗りつぶし、文字を main_bg で描画）
        graphics_context_set_fill_color(ctx, main_fg);
        graphics_fill_rect(ctx, GRect(0, y0, wt, h), 0, GCornerNone);
        title_color = main_bg;
        val_color = main_bg;
        unit_color = main_bg;
    } else {
        // 下段（タイトルと数値は main_fg、単位はアクションバーのアイコンと同色）
        title_color = main_fg;
        val_color = main_fg;
#if defined(PBL_COLOR)
        GColor pc = (GColor){.argb = ui_color_picker_get_personal_color_argb()};
        bool use_black = gcolor_equal(gcolor_legible_over(pc), GColorBlack);
        unit_color = use_black ? GColorBlack : GColorWhite;
#else
        unit_color = GColorWhite;
#endif
    }

    int margin_x = 4;
    int title_y = y0 + 2;
    int unit_y = y0 + h - 18;

    // タイトル（左上：有効幅の左端）
    graphics_context_set_text_color(ctx, title_color);
    graphics_draw_text(ctx, page->name, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
        GRect(margin_x, title_y, (active_w / 2) - margin_x, 16), 
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // 単位（右下：中段・下段ともに画面の一番右端揃え！）
    if (page->unit[0] != 0) {
        graphics_context_set_text_color(ctx, unit_color);
        graphics_draw_text(ctx, page->unit, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
            GRect(0, unit_y, wt - margin_x, 16), 
            GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    }

    // 数値（アクションバーを除いた有効横幅の中央に配置）
    GFont num_font;
    int val_y;
    int val_h;

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    num_font = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
    val_y = y0 + (h / 2) - 34;
    val_h = 70;
#elif defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT)
    num_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    val_y = y0 + (h / 2) - 22;
    val_h = 48;
#elif defined(PBL_PLATFORM_CHALK)
    num_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    val_y = y0 + (h / 2) - 22;
    val_h = 48;
#else // Aplite
    num_font = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
    val_y = y0 + (h / 2) - 16;
    val_h = 36;
#endif

    graphics_context_set_text_color(ctx, val_color);
    graphics_draw_text(ctx, page->value, num_font, 
        GRect(0, val_y, active_w, val_h), 
        GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void draw_line_plot(GContext *ctx, int pl, int bw, int base_y, int mah, 
                           int count, int plot_min, int plot_max, bool invert_y, bool zero_is_bottom) {
    int plot_range = plot_max - plot_min;
    if (plot_range <= 0) plot_range = 1;
    int prev_x = -1, prev_y = -1;

    for (int i = 0; i < count; i++) {
        int x = pl + i * bw + (bw / 2);
        int pt = graph_data_get_point(i);
        int y;
        if (zero_is_bottom && pt <= 0) {
            y = base_y;
        } else {
            int val = pt;
            if (val > plot_max) val = plot_max;
            if (val < plot_min) val = plot_min;
            int bh = ((val - plot_min) * mah) / plot_range;
            y = invert_y ? (base_y - mah + bh) : (base_y - bh);
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
}

static void draw_bar_plot(GContext *ctx, int pl, int bw, int base_y, int mah, 
                          int count, int plot_min, int plot_max, bool min_bar_height) {
    int plot_range = plot_max - plot_min;
    if (plot_range <= 0) plot_range = 1;
    int bar_w = bw - 1;
    if (bar_w < 1) bar_w = 1;

    for (int i = 0; i < count; i++) {
        int val = graph_data_get_point(i);
        if (val <= 0) continue;
        if (val > plot_max) val = plot_max;
        if (val < plot_min) val = plot_min;
        int bh = ((val - plot_min) * mah) / plot_range;
        if (min_bar_height && bh < 2) bh = 2;
        if (bh <= 0) continue;
        graphics_fill_rect(ctx, GRect(pl + i * bw, base_y - bh, bar_w, bh), 0, GCornerNone);
    }
}

static void draw_side_labels(GContext *ctx, int active_w, int lbl_margin_r, int base_y, 
                             const char *top_str, const char *bot_str, int lbl_w) {
    int lbl_x = active_w - lbl_w - (lbl_margin_r - 4);
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    if (top_str) {
        graphics_draw_text(ctx, top_str, font, GRect(lbl_x, 18, lbl_w, 16), 0, GTextAlignmentRight, NULL);
    }
    if (bot_str) {
        graphics_draw_text(ctx, bot_str, font, GRect(lbl_x, base_y - 16, lbl_w, 16), 0, GTextAlignmentRight, NULL);
    }
}

void ui_graph_layer_update_proc(Layer *layer, GContext *ctx, uint8_t app_state, GColor fg_color) {
    if (app_state < 3 && app_state != 6) return;
    // ワークアウト終了画面(5)またはマーキー表示中は下段項目を非表示
    if (ui_marquee_is_active()) return;
    
    GRect b = layer_get_bounds(layer);
    int wt = b.size.w;
#if defined(PBL_ROUND)
    int active_w = wt;
#else
    int active_w = wt - ACTION_BAR_WIDTH;
#endif

    graphics_context_set_fill_color(ctx, fg_color);
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_context_set_text_color(ctx, fg_color);

    // 下段がテキスト項目モードの場合
    if (!graph_data_is_lower_graph_mode()) {
        const LowerPageData *page = graph_data_get_current_lower_page();
        if (page && page->name[0] != 0) {
            ui_metric_render_page(ctx, GRect(0, 0, active_w, b.size.h), page, false, GColorBlack, fg_color, wt);
        }
        return;
    }

    // グラフ描画（アクションバー手前までの有効幅にプロット）
#if defined(PBL_PLATFORM_CHALK)
    int graph_margin_l = 28;
    int graph_margin_r = 28;
    int bottom_margin = 12;
    int lbl_margin_l = 18;
    int lbl_margin_r = 54;
#elif defined(PBL_PLATFORM_GABBRO)
    int graph_margin_l = 38;
    int graph_margin_r = 38;
    int bottom_margin = 16;
    int lbl_margin_l = 24;
    int lbl_margin_r = 70;
#else
    int graph_margin_l = 4;
    int graph_margin_r = 0;
    int bottom_margin = 0;
    int lbl_margin_l = 4;
    int lbl_margin_r = 4;
#endif

    int pl = graph_margin_l;
    int tm = 16;
    int ah = b.size.h - tm - bottom_margin;
    int dw = active_w - graph_margin_l - graph_margin_r;
    int base_y = b.size.h - bottom_margin;

    int graph_id = graph_data_get_id();
    int graph_count = graph_data_get_count();

    int bw = dw / (graph_count > 0 ? graph_count : 1);
    if (bw < 2) bw = 2; 

    bool show_labels = !ui_marquee_is_active();
    if (show_labels) {
        graphics_draw_text(ctx, graph_data_get_y_label(), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
            GRect(lbl_margin_l, 4, active_w - lbl_margin_l - lbl_margin_r, 16), 0, GTextAlignmentLeft, NULL);
        graphics_draw_text(ctx, graph_data_get_x_label(), fonts_get_system_font(FONT_KEY_GOTHIC_14), 
            GRect(lbl_margin_l, 4, active_w - lbl_margin_l - lbl_margin_r, 16), 0, GTextAlignmentRight, NULL);
    }
    if (graph_count <= 0) return;

    int mah = (ah * 70) / 100;

    // データ全体の min / max 探索
    int p_min = 999999, p_max = -999999;
    for (int i = 0; i < graph_count; i++) {
        int pt = graph_data_get_point(i);
        if (graph_id == 3 || pt > 0) {
            if (pt > p_max) p_max = pt;
            if (pt < p_min) p_min = pt;
        }
    }

    int plot_min = 0, plot_max = 1;

    if (graph_id == 0) { // PACE (折れ線・Y軸反転)
        if (p_min == 999999) { p_min = 0; p_max = 1; }
        if (p_max == p_min) p_max = p_min + 1;
        int margin = (p_max - p_min) / 4;
        if (margin < 10) margin = 10;
        plot_min = (p_min > margin) ? p_min - margin : 0;
        plot_max = p_max + margin;
        draw_line_plot(ctx, pl, bw, base_y, mah, graph_count, plot_min, plot_max, true, true);
    } else if (graph_id == 1) { // SPEED (棒グラフ・0基準)
        if (p_max < 1) p_max = 1;
        draw_bar_plot(ctx, pl, bw, base_y, mah, graph_count, 0, p_max, false);
    } else if (graph_id == 2) { // HR (棒グラフ・オートスケール)
        if (p_min == 999999) { p_min = 60; p_max = 120; }
        if (p_max == p_min) p_max = p_min + 10;
        plot_min = (p_min > 20) ? p_min - 20 : 0;
        plot_max = p_max + 10;
        draw_bar_plot(ctx, pl, bw, base_y, mah, graph_count, plot_min, plot_max, true);
    } else if (graph_id == 3) { // ELEVATION (折れ線・mラベル)
        if (p_min == 999999) { p_min = 0; p_max = 100; }
        if (p_max == p_min) { p_max += 10; p_min -= 10; }
        int margin = (p_max - p_min) / 4;
        if (margin < 5) margin = 5;
        plot_min = p_min - margin;
        plot_max = p_max + margin;
        draw_line_plot(ctx, pl, bw, base_y, mah, graph_count, plot_min, plot_max, false, false);
        if (show_labels) {
            char max_str[16], min_str[16];
            snprintf(max_str, 16, "%dm", p_max);
            snprintf(min_str, 16, "%dm", p_min);
            draw_side_labels(ctx, active_w, lbl_margin_r, base_y, max_str, min_str, 42);
        }
    } else if (graph_id == 4) { // CADENCE (折れ線・数値ラベル)
        if (p_min == 999999) { p_min = 60; p_max = 180; }
        if (p_max == p_min) p_max = p_min + 1;
        int margin = (p_max - p_min) / 4;
        if (margin < 5) margin = 5;
        plot_min = (p_min > margin) ? p_min - margin : 0;
        plot_max = p_max + margin;
        draw_line_plot(ctx, pl, bw, base_y, mah, graph_count, plot_min, plot_max, false, true);
        if (show_labels) {
            char max_str[16], min_str[16];
            snprintf(max_str, 16, "%d", p_max);
            snprintf(min_str, 16, "%d", p_min);
            draw_side_labels(ctx, active_w, lbl_margin_r, base_y, max_str, min_str, 28);
        }
    } else if (graph_id == 5) { // CALORIES (棒グラフ・MAXラベル)
        if (p_max < 1) p_max = 1;
        draw_bar_plot(ctx, pl, bw, base_y, mah, graph_count, 0, p_max, false);
        if (show_labels) {
            draw_side_labels(ctx, active_w, lbl_margin_r, base_y, graph_data_get_max_label(), NULL, 48);
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
#if defined(PBL_ROUND)
            int active_w = wt;
#else
            int active_w = wt - ACTION_BAR_WIDTH;
#endif
            ui_metric_render_page(ctx, GRect(0, upper_h, active_w, mid_h), page, true, main_bg, main_fg, wt);
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
