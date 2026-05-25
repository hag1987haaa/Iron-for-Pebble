import sys

with open('src/c/main.c', 'r') as f:
    text = f.read()

# 1. Remove FONT_HUGE_TIME and FONT_LONG_TIME macros
old_macros = '''/* ==========================================================
   定数・マクロ・フォント定義
   ========================================================== */
#if defined(PBL_PLATFORM_EMERY)
    #define FONT_HUGE_TIME FONT_KEY_LECO_60_NUMBERS_AM_PM 
    #define FONT_LONG_TIME FONT_KEY_LECO_42_NUMBERS
#else
    #define FONT_HUGE_TIME FONT_KEY_LECO_42_NUMBERS
    #define FONT_LONG_TIME FONT_KEY_LECO_32_BOLD_NUMBERS
#endif'''

new_macros = '''/* ==========================================================
   定数・マクロ・フォント定義
   ========================================================== */'''
if old_macros not in text: print("Failed to find macros")
text = text.replace(old_macros, new_macros)

# 2. Update main_window_load for round
old_round = '''#if defined(PBL_ROUND)
    int w = wt - ACTION_BAR_WIDTH;
    int mid_h = (wt >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    int lower_h = h - mid_h - upper_h;
    
    s_font_huge_time_key = FONT_KEY_LECO_42_NUMBERS;
    s_font_long_time_key = FONT_KEY_LECO_32_BOLD_NUMBERS;

    if (wt >= 260) {
        s_font_colon_key = FONT_KEY_GOTHIC_28_BOLD;
        s_font_mid_data_key = FONT_KEY_GOTHIC_18_BOLD;
    } else {
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
        y3_base = upper_h - 48; y3_colon = y3_base + 12;
        h5w = 40; c1w = 10; m5w = 40; c2w = 10; s5w = 40;
        y5_base = upper_h - 40; y5_colon = y5_base + 8;
    } else {
        m3w = 52; c3w = 10; s3w = 52;
        y3_base = upper_h - 48; y3_colon = y3_base + 12;
        h5w = 36; c1w = 8; m5w = 36; c2w = 8; s5w = 36;
        y5_base = upper_h - 40; y5_colon = y5_base + 8;
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
    s_rect_sec_3 = GRect(x3, y3_base, s3w, 80);'''

new_round = '''#if defined(PBL_ROUND)
    int w = wt - ACTION_BAR_WIDTH;
    int mid_h = (wt >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    int lower_h = h - mid_h - upper_h;
    
    if (wt >= 260) {
        s_font_huge_time_key = "RESOURCE_ID_LECO_60_NUMBERS_AM_PM";
        s_font_long_time_key = "RESOURCE_ID_LECO_42_NUMBERS";
        s_font_colon_key = FONT_KEY_GOTHIC_28_BOLD;
        s_font_mid_data_key = FONT_KEY_GOTHIC_18_BOLD;
    } else {
        s_font_huge_time_key = "RESOURCE_ID_LECO_42_NUMBERS";
        s_font_long_time_key = "RESOURCE_ID_LECO_32_BOLD_NUMBERS";
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
        m3w = 64; c3w = 14; s3w = 64;
        y3_base = upper_h - 62; y3_colon = y3_base + 16;
        h5w = 42; c1w = 12; m5w = 42; c2w = 12; s5w = 42;
        y5_base = upper_h - 46; y5_colon = y5_base + 10;
    } else {
        m3w = 44; c3w = 10; s3w = 44;
        y3_base = upper_h - 46; y3_colon = y3_base + 10;
        h5w = 34; c1w = 6; m5w = 34; c2w = 6; s5w = 34;
        y5_base = upper_h - 36; y5_colon = y5_base + 6;
    }
    
    int x3 = (wt - (m3w + c3w + s3w)) / 2; 
    int x5 = (wt - (h5w + c1w + m5w + c2w + s5w)) / 2;
    
    s_rect_hour_5 = GRect(x5, y5_base, h5w, 60); x5 += h5w;
    s_rect_col1_5 = GRect(x5, y5_colon, c1w, 60); x5 += c1w;
    s_rect_min_5 = GRect(x5, y5_base, m5w, 60); x5 += m5w;
    s_rect_col2_5 = GRect(x5, y5_colon, c2w, 60); x5 += c2w;
    s_rect_sec_5 = GRect(x5, y5_base, s5w, 60);
    
    s_rect_min_3 = GRect(x3, y3_base, m3w, 80); x3 += m3w;
    s_rect_col2_3 = GRect(x3, y3_colon, c3w, 80); x3 += c3w;
    s_rect_sec_3 = GRect(x3, y3_base, s3w, 80);'''
if old_round not in text: print("Failed to find old round")
text = text.replace(old_round, new_round)

# 3. Update non-round logic
old_nonround = '''#else
    int w = wt - ACTION_BAR_WIDTH;
    int h3 = h / 3; 

    s_font_huge_time_key = FONT_HUGE_TIME;
    s_font_long_time_key = FONT_LONG_TIME;'''

new_nonround = '''#else
    int w = wt - ACTION_BAR_WIDTH;
    int h3 = h / 3; 

#if defined(PBL_PLATFORM_EMERY)
    s_font_huge_time_key = "RESOURCE_ID_LECO_60_NUMBERS_AM_PM";
    s_font_long_time_key = "RESOURCE_ID_LECO_42_NUMBERS";
#else
    s_font_huge_time_key = "RESOURCE_ID_LECO_42_NUMBERS";
    s_font_long_time_key = "RESOURCE_ID_LECO_32_BOLD_NUMBERS";
#endif'''
if old_nonround not in text: print("Failed to find non-round")
text = text.replace(old_nonround, new_nonround)

with open('src/c/main.c', 'w') as f:
    f.write(text)

print("Patch 3 Complete")
