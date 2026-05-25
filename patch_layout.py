import sys

with open('src/c/main.c', 'r') as f:
    content = f.read()

# 1. trigger_marquee
content = content.replace('''#if defined(PBL_PLATFORM_CHALK)
    int w = layer_get_bounds(wl).size.w;
#else
    int w = layer_get_bounds(wl).size.w - ACTION_BAR_WIDTH;
#endif''', '''#if defined(PBL_ROUND)
    int w = layer_get_bounds(wl).size.w;
#else
    int w = layer_get_bounds(wl).size.w - ACTION_BAR_WIDTH;
#endif''')

# 2. create_color_picker_layer
old_color_picker = '''static void create_color_picker_layer() {
    if (s_color_picker_layer != NULL) return;
    Layer *wl = window_get_root_layer(s_main_window);
    GRect b = layer_get_bounds(wl);
    int h3 = b.size.h / 3;
#if defined(PBL_PLATFORM_CHALK)
    int w = b.size.w;
#else
    int w = b.size.w - ACTION_BAR_WIDTH;
#endif
    s_color_picker_layer = layer_create(GRect(0, h3, w, b.size.h - h3));
    layer_set_update_proc(s_color_picker_layer, color_picker_update_proc);
    layer_add_child(wl, s_color_picker_layer);
}'''

new_color_picker = '''static void create_color_picker_layer() {
    if (s_color_picker_layer != NULL) return;
    Layer *wl = window_get_root_layer(s_main_window);
    GRect b = layer_get_bounds(wl);
#if defined(PBL_ROUND)
    int w = b.size.w;
    int upper_h = (w >= 260) ? 75 : 50;
    s_color_picker_layer = layer_create(GRect(0, upper_h, w, b.size.h - upper_h));
#else
    int h3 = b.size.h / 3;
    int w = b.size.w - ACTION_BAR_WIDTH;
    s_color_picker_layer = layer_create(GRect(0, h3, w, b.size.h - h3));
#endif
    layer_set_update_proc(s_color_picker_layer, color_picker_update_proc);
    if (s_action_bar) {
        layer_insert_below_sibling(s_color_picker_layer, action_bar_layer_get_layer(s_action_bar));
    } else {
        layer_add_child(wl, s_color_picker_layer);
    }
}'''
content = content.replace(old_color_picker, new_color_picker)

# 3. create_marquee_layers
old_marquee = '''static void create_marquee_layers() {
    if (s_msg_container_layer != NULL) return;
    
    Layer *wl = window_get_root_layer(s_main_window);
    GRect b = layer_get_bounds(wl);
    
#if defined(PBL_PLATFORM_CHALK)
    int w = b.size.w;
    int h3 = b.size.h / 3;
    s_msg_container_layer = layer_create(GRect(0, h3 * 2 + 5, w, 24));
#else
    int w = b.size.w - ACTION_BAR_WIDTH;
    int h3 = b.size.h / 3;
    s_msg_container_layer = layer_create(GRect(0, h3 * 2 + 1, w, 24));
#endif

    layer_set_update_proc(s_msg_container_layer, msg_container_update_proc);
    layer_add_child(wl, s_msg_container_layer);
    
    s_msg_layer = text_layer_create(GRect(w, -2, 450, 24));
    text_layer_set_font(s_msg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_background_color(s_msg_layer, GColorClear);
    layer_add_child(s_msg_container_layer, text_layer_get_layer(s_msg_layer));
}'''

new_marquee = '''static void create_marquee_layers() {
    if (s_msg_container_layer != NULL) return;
    
    Layer *wl = window_get_root_layer(s_main_window);
    GRect b = layer_get_bounds(wl);
    
#if defined(PBL_ROUND)
    int w = b.size.w;
    int upper_h = (w >= 260) ? 75 : 50;
    int mid_h = (w >= 260) ? 65 : 45;
    s_msg_container_layer = layer_create(GRect(0, upper_h + mid_h + (w >= 260 ? 5 : 2), w, 24));
#else
    int w = b.size.w - ACTION_BAR_WIDTH;
    int h3 = b.size.h / 3;
    s_msg_container_layer = layer_create(GRect(0, h3 * 2 + 1, w, 24));
#endif

    layer_set_update_proc(s_msg_container_layer, msg_container_update_proc);
    
    if (s_action_bar) {
        layer_insert_below_sibling(s_msg_container_layer, action_bar_layer_get_layer(s_action_bar));
    } else {
        layer_add_child(wl, s_msg_container_layer);
    }
    
    s_msg_layer = text_layer_create(GRect(w, -2, 450, 24));
    text_layer_set_font(s_msg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_background_color(s_msg_layer, GColorClear);
    layer_add_child(s_msg_container_layer, text_layer_get_layer(s_msg_layer));
}'''
content = content.replace(old_marquee, new_marquee)

# 4. update_ui_state (fonts)
old_ui_fonts = '''    if (s_is_long_workout) {
        text_layer_set_font(s_time_hour_layer, fonts_get_system_font(FONT_LONG_TIME));
        text_layer_set_font(s_time_min_layer, fonts_get_system_font(FONT_LONG_TIME));
        text_layer_set_font(s_time_sec_layer, fonts_get_system_font(FONT_LONG_TIME));
        
        layer_set_frame(text_layer_get_layer(s_time_hour_layer), s_rect_hour_5);
        layer_set_frame(text_layer_get_layer(s_time_colon1_layer), s_rect_col1_5);
        layer_set_frame(text_layer_get_layer(s_time_min_layer), s_rect_min_5);
        layer_set_frame(text_layer_get_layer(s_time_colon2_layer), s_rect_col2_5);
        layer_set_frame(text_layer_get_layer(s_time_sec_layer), s_rect_sec_5);
    } else {
        text_layer_set_font(s_time_min_layer, fonts_get_system_font(FONT_HUGE_TIME));
        text_layer_set_font(s_time_sec_layer, fonts_get_system_font(FONT_HUGE_TIME));
        
        layer_set_frame(text_layer_get_layer(s_time_min_layer), s_rect_min_3);
        layer_set_frame(text_layer_get_layer(s_time_colon2_layer), s_rect_col2_3);
        layer_set_frame(text_layer_get_layer(s_time_sec_layer), s_rect_sec_3);
    }'''

new_ui_fonts = '''    if (s_is_long_workout) {
        text_layer_set_font(s_time_hour_layer, fonts_get_system_font(s_font_long_time_key));
        text_layer_set_font(s_time_min_layer, fonts_get_system_font(s_font_long_time_key));
        text_layer_set_font(s_time_sec_layer, fonts_get_system_font(s_font_long_time_key));
        
        layer_set_frame(text_layer_get_layer(s_time_hour_layer), s_rect_hour_5);
        layer_set_frame(text_layer_get_layer(s_time_colon1_layer), s_rect_col1_5);
        layer_set_frame(text_layer_get_layer(s_time_min_layer), s_rect_min_5);
        layer_set_frame(text_layer_get_layer(s_time_colon2_layer), s_rect_col2_5);
        layer_set_frame(text_layer_get_layer(s_time_sec_layer), s_rect_sec_5);
    } else {
        text_layer_set_font(s_time_min_layer, fonts_get_system_font(s_font_huge_time_key));
        text_layer_set_font(s_time_sec_layer, fonts_get_system_font(s_font_huge_time_key));
        
        layer_set_frame(text_layer_get_layer(s_time_min_layer), s_rect_min_3);
        layer_set_frame(text_layer_get_layer(s_time_colon2_layer), s_rect_col2_3);
        layer_set_frame(text_layer_get_layer(s_time_sec_layer), s_rect_sec_3);
    }'''
content = content.replace(old_ui_fonts, new_ui_fonts)

# 5. mid_bg_update_proc
old_mid_bg = '''static void mid_bg_update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, s_current_main_fg);
    graphics_context_set_fill_color(ctx, s_current_main_fg);

#if defined(PBL_PLATFORM_CHALK)
    int w = b.size.w - ACTION_BAR_WIDTH;
    int h3 = 60; 
    int mx = 20; 
    
    graphics_draw_line(ctx, GPoint(mx, h3), GPoint(w - mx, h3));
    graphics_draw_line(ctx, GPoint(mx, h3 * 2), GPoint(w - mx, h3 * 2));
    
    int r1y = h3 + 17;
    int r2y = h3 + 45;
    
    int lx = mx + 10;
    int rx = (w / 2) + 15; 
    
    graphics_fill_circle(ctx, GPoint(lx, r1y), 3);
    graphics_draw_line(ctx, GPoint(lx - 2, r1y + 2), GPoint(lx, r1y + 6));
    graphics_draw_line(ctx, GPoint(lx + 2, r1y + 2), GPoint(lx, r1y + 6));
    
    graphics_fill_rect(ctx, GRect(rx - 1, r1y, 3, 5), 1, GCornerNone);
    graphics_fill_circle(ctx, GPoint(rx + 2, r1y - 2), 1);
    graphics_fill_circle(ctx, GPoint(rx, r1y - 3), 1);

    graphics_fill_circle(ctx, GPoint(lx - 2, r2y), 2);
    graphics_fill_circle(ctx, GPoint(lx + 2, r2y), 2);
    graphics_draw_line(ctx, GPoint(lx - 4, r2y + 1), GPoint(lx, r2y + 5));
    graphics_draw_line(ctx, GPoint(lx + 4, r2y + 1), GPoint(lx, r2y + 5));

    graphics_draw_circle(ctx, GPoint(rx, r2y), 4);
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx, r2y - 2));
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx + 2, r2y));

#else'''

new_mid_bg = '''static void mid_bg_update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, s_current_main_fg);
    graphics_context_set_fill_color(ctx, s_current_main_fg);

#if defined(PBL_ROUND)
    int w = b.size.w - ACTION_BAR_WIDTH;
    int upper_h = (b.size.w >= 260) ? 75 : 50;
    int mid_h = (b.size.w >= 260) ? 65 : 45;
    int mx = (b.size.w >= 260) ? 30 : 20; 
    
    graphics_draw_line(ctx, GPoint(mx, upper_h), GPoint(w - mx, upper_h));
    graphics_draw_line(ctx, GPoint(mx, upper_h + mid_h), GPoint(w - mx, upper_h + mid_h));
    
    int row_h = (b.size.w >= 260) ? 28 : 18;
    int base_y = upper_h + (mid_h - row_h * 2) / 2 + (b.size.w >= 260 ? 12 : 9);
    int r1y = base_y;
    int r2y = base_y + row_h;
    
    int lx = mx + ((b.size.w >= 260) ? 10 : -4);
    int rx = (w / 2) + ((b.size.w >= 260) ? 15 : 12); 
    
    graphics_fill_circle(ctx, GPoint(lx, r1y), 3);
    graphics_draw_line(ctx, GPoint(lx - 2, r1y + 2), GPoint(lx, r1y + 6));
    graphics_draw_line(ctx, GPoint(lx + 2, r1y + 2), GPoint(lx, r1y + 6));
    
    graphics_fill_rect(ctx, GRect(rx - 1, r1y, 3, 5), 1, GCornerNone);
    graphics_fill_circle(ctx, GPoint(rx + 2, r1y - 2), 1);
    graphics_fill_circle(ctx, GPoint(rx, r1y - 3), 1);

    graphics_fill_circle(ctx, GPoint(lx - 2, r2y), 2);
    graphics_fill_circle(ctx, GPoint(lx + 2, r2y), 2);
    graphics_draw_line(ctx, GPoint(lx - 4, r2y + 1), GPoint(lx, r2y + 5));
    graphics_draw_line(ctx, GPoint(lx + 4, r2y + 1), GPoint(lx, r2y + 5));

    graphics_draw_circle(ctx, GPoint(rx, r2y), 4);
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx, r2y - 2));
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx + 2, r2y));

#else'''
content = content.replace(old_mid_bg, new_mid_bg)

# 6. main_window_load
old_main_window_load = '''/* ==========================================================
   ウィンドウロード・メイン関数
   ========================================================== */
static void main_window_load(Window *window) {
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
    
    int wt = b.size.w;
    int h = b.size.h;

    s_is_small_screen = (wt <= 144);
    s_is_round_screen = (wt == 180 && h == 180); 
    s_has_hr_sensor = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL)) & HealthServiceAccessibilityMaskAvailable;

#if defined(PBL_PLATFORM_CHALK)
    int w = wt - ACTION_BAR_WIDTH;
    int h3 = 60; // h/3
    
    s_action_bar = action_bar_layer_create();
    action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
    action_bar_layer_add_to_window(s_action_bar, window);

    s_mid_bg_layer = layer_create(GRect(0, 0, wt, h));
    layer_set_update_proc(s_mid_bg_layer, mid_bg_update_proc);
    layer_add_child(wl, s_mid_bg_layer);
    
    s_graph_layer = layer_create(GRect(0, h3 * 2, w, h - (h3 * 2)));
    layer_set_update_proc(s_graph_layer, graph_update_proc);
    layer_add_child(wl, s_graph_layer);

    int by = (h3 / 2) - 20;
    int m3w = 52, c3w = 8, s3w = 52;
    int x3 = (w - (m3w + c3w + s3w)) / 2 + 10; 
    
    int y3_base = 10;
    int y3_colon = 24;
    
    int y5_base = 20;
    int y5_colon = 24;
    
    int h5w = 20, c1w = 8, m5w = 32, c2w = 8, s5w = 32;
    int x5 = (w - (h5w + c1w + m5w + c2w + s5w)) / 2 + 10;
    
    s_rect_hour_5 = GRect(x5, y5_base, h5w, 50); x5 += h5w;
    s_rect_col1_5 = GRect(x5, y5_colon, c1w, 50); x5 += c1w;
    s_rect_min_5 = GRect(x5, y5_base, m5w, 50); x5 += m5w;
    s_rect_col2_5 = GRect(x5, y5_colon, c2w, 50); x5 += c2w;
    s_rect_sec_5 = GRect(x5, y5_base, s5w, 50);
    
    s_rect_min_3 = GRect(x3, y3_base, m3w, 80); x3 += m3w;
    s_rect_col2_3 = GRect(x3, y3_colon, c3w, 50); x3 += c3w;
    s_rect_sec_3 = GRect(x3, y3_base, s3w, 80);

    int tw = (w / 2) - 5; 
    s_dist_layer = text_layer_create(GRect(10, h3 + 5, tw, 28));
    s_step_layer = text_layer_create(GRect((w / 2) + 5, h3 + 5, tw, 28));
    s_hr_layer = text_layer_create(GRect(10, h3 + 33, tw, 28));
    s_clock_layer = text_layer_create(GRect((w / 2) + 5, h3 + 33, tw, 28));

#else
    int w = wt - ACTION_BAR_WIDTH;
    int h3 = h / 3; 

    s_action_bar = action_bar_layer_create();
    action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
    action_bar_layer_add_to_window(s_action_bar, window);
    
    s_mid_bg_layer = layer_create(GRect(0, 0, wt, h));
    layer_set_update_proc(s_mid_bg_layer, mid_bg_update_proc);
    layer_add_child(wl, s_mid_bg_layer);
    
    s_graph_layer = layer_create(GRect(0, h3 * 2, w, h - (h3 * 2)));
    layer_set_update_proc(s_graph_layer, graph_update_proc);
    layer_add_child(wl, s_graph_layer);

    int by = (h3 / 2) - 20;
    int m3w = s_is_small_screen ? 52 : 78;
    int c3w = s_is_small_screen ? 8 : 10;
    int s3w = s_is_small_screen ? 52 : 78;
    int x3 = (w - (m3w + c3w + s3w)) / 2;
    
    int y3_base = by - (s_is_small_screen ? 0 : 15);
    int y3_colon = y3_base + (s_is_small_screen ? 14 : 25);
    
    int y5_base = y3_base + (s_is_small_screen ? 4 : 15);
    int y5_colon = y5_base + (s_is_small_screen ? 8 : 6);
    int h5w = s_is_small_screen ? 20 : 36;
    int c1w = s_is_small_screen ? 6 : 6;
    int m5w = s_is_small_screen ? 40 : 56;
    int c2w = s_is_small_screen ? 6 : 6;
    int s5w = s_is_small_screen ? 40 : 56;
    int x5 = (w - (h5w + c1w + m5w + c2w + s5w)) / 2;
    
    s_rect_hour_5 = GRect(x5, y5_base, h5w, 50); x5 += h5w;
    s_rect_col1_5 = GRect(x5, y5_colon, c1w, 50); x5 += c1w;
    s_rect_min_5 = GRect(x5, y5_base, m5w, 50); x5 += m5w;
    s_rect_col2_5 = GRect(x5, y5_colon, c2w, 50); x5 += c2w;
    s_rect_sec_5 = GRect(x5, y5_base, s5w, 50);
    
    s_rect_min_3 = GRect(x3, y3_base, m3w, 80); x3 += m3w;
    s_rect_col2_3 = GRect(x3, y3_colon, c3w, 50); x3 += c3w;
    s_rect_sec_3 = GRect(x3, y3_base, s3w, 80);

    int mx = s_is_small_screen ? 5 : 10;
    int tw = (w / 2) - mx - 14; 
    
    s_dist_layer = text_layer_create(GRect(mx + 16, h3 + 5, tw, 28));
    s_step_layer = text_layer_create(GRect((w / 2) + 16, h3 + 5, tw, 28));
    s_hr_layer = text_layer_create(GRect(mx + 16, h3 + 33, tw, 28));
    s_clock_layer = text_layer_create(GRect((w / 2) + 16, h3 + 33, tw, 28));
#endif

    // 共通レイヤー設定
    s_time_hour_layer = text_layer_create(s_rect_hour_5);
    text_layer_set_background_color(s_time_hour_layer, GColorClear);
    text_layer_set_text_alignment(s_time_hour_layer, GTextAlignmentRight);
    text_layer_set_text(s_time_hour_layer, s_time_hour_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_hour_layer));
    
    s_time_colon1_layer = text_layer_create(s_rect_col1_5);
    text_layer_set_background_color(s_time_colon1_layer, GColorClear);
    text_layer_set_text_alignment(s_time_colon1_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_colon1_layer, fonts_get_system_font(s_is_small_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(s_time_colon1_layer, ":");
    layer_add_child(wl, text_layer_get_layer(s_time_colon1_layer));
    
    s_time_min_layer = text_layer_create(s_rect_min_3);
    text_layer_set_background_color(s_time_min_layer, GColorClear);
    text_layer_set_text_alignment(s_time_min_layer, GTextAlignmentCenter);
    text_layer_set_text(s_time_min_layer, s_time_min_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_min_layer));
    
    s_time_colon2_layer = text_layer_create(s_rect_col2_3);
    text_layer_set_background_color(s_time_colon2_layer, GColorClear);
    text_layer_set_text_alignment(s_time_colon2_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_colon2_layer, fonts_get_system_font(s_is_small_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(s_time_colon2_layer, ":");
    layer_add_child(wl, text_layer_get_layer(s_time_colon2_layer));
    
    s_time_sec_layer = text_layer_create(s_rect_sec_3);
    text_layer_set_background_color(s_time_sec_layer, GColorClear);
    text_layer_set_text_alignment(s_time_sec_layer, GTextAlignmentCenter);
    text_layer_set_text(s_time_sec_layer, s_time_sec_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_sec_layer));
    
    const char *mf = s_is_small_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD;
    text_layer_set_font(s_dist_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_dist_layer, GColorClear);
    text_layer_set_text_alignment(s_dist_layer, GTextAlignmentLeft);
    text_layer_set_text(s_dist_layer, s_dist_buf);
    layer_add_child(wl, text_layer_get_layer(s_dist_layer));
    
    text_layer_set_font(s_step_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_step_layer, GColorClear);
    text_layer_set_text_alignment(s_step_layer, GTextAlignmentLeft);
    text_layer_set_text(s_step_layer, s_step_buf);
    layer_add_child(wl, text_layer_get_layer(s_step_layer));
    
    text_layer_set_font(s_hr_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_hr_layer, GColorClear);
    text_layer_set_text_alignment(s_hr_layer, GTextAlignmentLeft);
    text_layer_set_text(s_hr_layer, s_hr_buf);
    layer_add_child(wl, text_layer_get_layer(s_hr_layer));
    
    text_layer_set_font(s_clock_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_clock_layer, GColorClear);
    text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
    text_layer_set_text(s_clock_layer, s_clock_buf);
    layer_add_child(wl, text_layer_get_layer(s_clock_layer));'''

new_main_window_load = '''/* ==========================================================
   ウィンドウロード・メイン関数
   ========================================================== */
static const char *s_font_huge_time_key;
static const char *s_font_long_time_key;
static const char *s_font_colon_key;
static const char *s_font_mid_data_key;

static void main_window_load(Window *window) {
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
    
    int wt = b.size.w;
    int h = b.size.h;

    s_is_small_screen = (wt <= 144);
    s_is_round_screen = (wt == 180 && h == 180) || (wt == 260 && h == 260) || defined(PBL_ROUND); 
    s_has_hr_sensor = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL)) & HealthServiceAccessibilityMaskAvailable;

#if defined(PBL_ROUND)
    int w = wt - ACTION_BAR_WIDTH;
    int upper_h, mid_h, lower_h;
    if (wt >= 260) {
        upper_h = 75;
        mid_h = 65;
        lower_h = h - upper_h - mid_h;
        s_font_huge_time_key = FONT_KEY_LECO_42_NUMBERS;
        s_font_long_time_key = FONT_KEY_LECO_32_BOLD_NUMBERS;
        s_font_colon_key = FONT_KEY_GOTHIC_28_BOLD;
        s_font_mid_data_key = FONT_KEY_GOTHIC_18_BOLD;
    } else {
        upper_h = 50;
        mid_h = 45;
        lower_h = h - upper_h - mid_h;
        s_font_huge_time_key = FONT_KEY_LECO_28_LIGHT_NUMBERS;
        s_font_long_time_key = FONT_KEY_GOTHIC_24_BOLD;
        s_font_colon_key = FONT_KEY_GOTHIC_24_BOLD;
        s_font_mid_data_key = FONT_KEY_GOTHIC_14_BOLD;
    }

    s_action_bar = action_bar_layer_create();
    action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
    action_bar_layer_add_to_window(s_action_bar, window);

    s_mid_bg_layer = layer_create(GRect(0, 0, wt, h));
    layer_set_update_proc(s_mid_bg_layer, mid_bg_update_proc);
    layer_add_child(wl, s_mid_bg_layer);
    
    s_graph_layer = layer_create(GRect(0, upper_h + mid_h, w, lower_h));
    layer_set_update_proc(s_graph_layer, graph_update_proc);
    layer_add_child(wl, s_graph_layer);

    int m3w, c3w, s3w, y3_base, y3_colon;
    int h5w, c1w, m5w, c2w, s5w, y5_base, y5_colon;
    if (wt >= 260) {
        m3w = 60; c3w = 12; s3w = 60;
        y3_base = 15; y3_colon = 25;
        h5w = 40; c1w = 10; m5w = 40; c2w = 10; s5w = 40;
        y5_base = 22; y5_colon = 28;
    } else {
        m3w = 46; c3w = 8; s3w = 46;
        y3_base = 10; y3_colon = 12;
        h5w = 30; c1w = 6; m5w = 30; c2w = 6; s5w = 30;
        y5_base = 12; y5_colon = 14;
    }
    
    int x3 = (w - (m3w + c3w + s3w)) / 2 + (wt >= 260 ? 10 : 8); 
    int x5 = (w - (h5w + c1w + m5w + c2w + s5w)) / 2 + (wt >= 260 ? 10 : 8);
    
    s_rect_hour_5 = GRect(x5, y5_base, h5w, 50); x5 += h5w;
    s_rect_col1_5 = GRect(x5, y5_colon, c1w, 50); x5 += c1w;
    s_rect_min_5 = GRect(x5, y5_base, m5w, 50); x5 += m5w;
    s_rect_col2_5 = GRect(x5, y5_colon, c2w, 50); x5 += c2w;
    s_rect_sec_5 = GRect(x5, y5_base, s5w, 50);
    
    s_rect_min_3 = GRect(x3, y3_base, m3w, 80); x3 += m3w;
    s_rect_col2_3 = GRect(x3, y3_colon, c3w, 50); x3 += c3w;
    s_rect_sec_3 = GRect(x3, y3_base, s3w, 80);

    int mx = (wt >= 260) ? 30 : 20; 
    int lx = mx + ((wt >= 260) ? 10 : -4);
    int rx = (w / 2) + ((wt >= 260) ? 15 : 12); 
    int tw = rx - lx - 14; 
    
    int row_h = (wt >= 260) ? 28 : 18;
    int row1_y = upper_h + (mid_h - row_h * 2) / 2 + (wt >= 260 ? -2 : 2); 
    int row2_y = row1_y + row_h;

    s_dist_layer = text_layer_create(GRect(lx + 8, row1_y, tw, row_h));
    s_step_layer = text_layer_create(GRect(rx + 8, row1_y, tw, row_h));
    s_hr_layer = text_layer_create(GRect(lx + 8, row2_y, tw, row_h));
    s_clock_layer = text_layer_create(GRect(rx + 8, row2_y, tw, row_h));

#else
    int w = wt - ACTION_BAR_WIDTH;
    int h3 = h / 3; 

    s_font_huge_time_key = FONT_HUGE_TIME;
    s_font_long_time_key = FONT_LONG_TIME;
    s_font_colon_key = s_is_small_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD;
    s_font_mid_data_key = s_is_small_screen ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD;

    s_action_bar = action_bar_layer_create();
    action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
    action_bar_layer_add_to_window(s_action_bar, window);
    
    s_mid_bg_layer = layer_create(GRect(0, 0, wt, h));
    layer_set_update_proc(s_mid_bg_layer, mid_bg_update_proc);
    layer_add_child(wl, s_mid_bg_layer);
    
    s_graph_layer = layer_create(GRect(0, h3 * 2, w, h - (h3 * 2)));
    layer_set_update_proc(s_graph_layer, graph_update_proc);
    layer_add_child(wl, s_graph_layer);

    int by = (h3 / 2) - 20;
    int m3w = s_is_small_screen ? 52 : 78;
    int c3w = s_is_small_screen ? 8 : 10;
    int s3w = s_is_small_screen ? 52 : 78;
    int x3 = (w - (m3w + c3w + s3w)) / 2;
    
    int y3_base = by - (s_is_small_screen ? 0 : 15);
    int y3_colon = y3_base + (s_is_small_screen ? 14 : 25);
    
    int y5_base = y3_base + (s_is_small_screen ? 4 : 15);
    int y5_colon = y5_base + (s_is_small_screen ? 8 : 6);
    int h5w = s_is_small_screen ? 20 : 36;
    int c1w = s_is_small_screen ? 6 : 6;
    int m5w = s_is_small_screen ? 40 : 56;
    int c2w = s_is_small_screen ? 6 : 6;
    int s5w = s_is_small_screen ? 40 : 56;
    int x5 = (w - (h5w + c1w + m5w + c2w + s5w)) / 2;
    
    s_rect_hour_5 = GRect(x5, y5_base, h5w, 50); x5 += h5w;
    s_rect_col1_5 = GRect(x5, y5_colon, c1w, 50); x5 += c1w;
    s_rect_min_5 = GRect(x5, y5_base, m5w, 50); x5 += m5w;
    s_rect_col2_5 = GRect(x5, y5_colon, c2w, 50); x5 += c2w;
    s_rect_sec_5 = GRect(x5, y5_base, s5w, 50);
    
    s_rect_min_3 = GRect(x3, y3_base, m3w, 80); x3 += m3w;
    s_rect_col2_3 = GRect(x3, y3_colon, c3w, 50); x3 += c3w;
    s_rect_sec_3 = GRect(x3, y3_base, s3w, 80);

    int mx = s_is_small_screen ? 5 : 10;
    int tw = (w / 2) - mx - 14; 
    
    s_dist_layer = text_layer_create(GRect(mx + 16, h3 + 5, tw, 28));
    s_step_layer = text_layer_create(GRect((w / 2) + 16, h3 + 5, tw, 28));
    s_hr_layer = text_layer_create(GRect(mx + 16, h3 + 33, tw, 28));
    s_clock_layer = text_layer_create(GRect((w / 2) + 16, h3 + 33, tw, 28));
#endif

    // 共通レイヤー設定
    s_time_hour_layer = text_layer_create(s_rect_hour_5);
    text_layer_set_background_color(s_time_hour_layer, GColorClear);
    text_layer_set_text_alignment(s_time_hour_layer, GTextAlignmentRight);
    text_layer_set_text(s_time_hour_layer, s_time_hour_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_hour_layer));
    
    s_time_colon1_layer = text_layer_create(s_rect_col1_5);
    text_layer_set_background_color(s_time_colon1_layer, GColorClear);
    text_layer_set_text_alignment(s_time_colon1_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_colon1_layer, fonts_get_system_font(s_font_colon_key));
    text_layer_set_text(s_time_colon1_layer, ":");
    layer_add_child(wl, text_layer_get_layer(s_time_colon1_layer));
    
    s_time_min_layer = text_layer_create(s_rect_min_3);
    text_layer_set_background_color(s_time_min_layer, GColorClear);
    text_layer_set_text_alignment(s_time_min_layer, GTextAlignmentCenter);
    text_layer_set_text(s_time_min_layer, s_time_min_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_min_layer));
    
    s_time_colon2_layer = text_layer_create(s_rect_col2_3);
    text_layer_set_background_color(s_time_colon2_layer, GColorClear);
    text_layer_set_text_alignment(s_time_colon2_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_colon2_layer, fonts_get_system_font(s_font_colon_key));
    text_layer_set_text(s_time_colon2_layer, ":");
    layer_add_child(wl, text_layer_get_layer(s_time_colon2_layer));
    
    s_time_sec_layer = text_layer_create(s_rect_sec_3);
    text_layer_set_background_color(s_time_sec_layer, GColorClear);
    text_layer_set_text_alignment(s_time_sec_layer, GTextAlignmentCenter);
    text_layer_set_text(s_time_sec_layer, s_time_sec_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_sec_layer));
    
    const char *mf = s_font_mid_data_key;
    text_layer_set_font(s_dist_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_dist_layer, GColorClear);
    text_layer_set_text_alignment(s_dist_layer, GTextAlignmentLeft);
    text_layer_set_text(s_dist_layer, s_dist_buf);
    layer_add_child(wl, text_layer_get_layer(s_dist_layer));
    
    text_layer_set_font(s_step_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_step_layer, GColorClear);
    text_layer_set_text_alignment(s_step_layer, GTextAlignmentLeft);
    text_layer_set_text(s_step_layer, s_step_buf);
    layer_add_child(wl, text_layer_get_layer(s_step_layer));
    
    text_layer_set_font(s_hr_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_hr_layer, GColorClear);
    text_layer_set_text_alignment(s_hr_layer, GTextAlignmentLeft);
    text_layer_set_text(s_hr_layer, s_hr_buf);
    layer_add_child(wl, text_layer_get_layer(s_hr_layer));
    
    text_layer_set_font(s_clock_layer, fonts_get_system_font(mf));
    text_layer_set_background_color(s_clock_layer, GColorClear);
    text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
    text_layer_set_text(s_clock_layer, s_clock_buf);
    layer_add_child(wl, text_layer_get_layer(s_clock_layer));'''
content = content.replace(old_main_window_load, new_main_window_load)

with open('src/c/main.c', 'w') as f:
    f.write(content)

print("Replacement Complete")
