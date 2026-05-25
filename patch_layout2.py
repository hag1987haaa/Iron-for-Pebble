import sys

with open('src/c/main.c', 'r') as f:
    text = f.read()

# 1. create_color_picker_layer
old_color_picker = '''#if defined(PBL_ROUND)
    int w = b.size.w;
    int upper_h = (w >= 260) ? 75 : 50;
    s_color_picker_layer = layer_create(GRect(0, upper_h, w, b.size.h - upper_h));'''

new_color_picker = '''#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    s_color_picker_layer = layer_create(GRect(0, upper_h, w, h - upper_h));'''
if old_color_picker not in text: print("Failed to find color picker")
text = text.replace(old_color_picker, new_color_picker)

# 2. create_marquee_layers
old_marquee = '''#if defined(PBL_ROUND)
    int w = b.size.w;
    int upper_h = (w >= 260) ? 75 : 50;
    int mid_h = (w >= 260) ? 65 : 45;
    s_msg_container_layer = layer_create(GRect(0, upper_h + mid_h + (w >= 260 ? 5 : 2), w, 24));'''

new_marquee = '''#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    s_msg_container_layer = layer_create(GRect(0, upper_h + mid_h + (w >= 260 ? 5 : 2), w, 24));'''
if old_marquee not in text: print("Failed to find marquee")
text = text.replace(old_marquee, new_marquee)

# 3. mid_bg_update_proc
old_mid_bg = '''#if defined(PBL_ROUND)
    int w = b.size.w - ACTION_BAR_WIDTH;
    int upper_h = (b.size.w >= 260) ? 75 : 50;
    int mid_h = (b.size.w >= 260) ? 65 : 45;
    int mx = (b.size.w >= 260) ? 30 : 20; '''

new_mid_bg = '''#if defined(PBL_ROUND)
    int w = b.size.w - ACTION_BAR_WIDTH;
    int h = b.size.h;
    int mid_h = (b.size.w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
    int mx = (b.size.w >= 260) ? 30 : 20; '''
if old_mid_bg not in text: print("Failed to find mid_bg")
text = text.replace(old_mid_bg, new_mid_bg)

# 4. main_window_load
old_main_load = '''#if defined(PBL_ROUND)
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
    }'''

new_main_load = '''#if defined(PBL_ROUND)
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
    }'''
if old_main_load not in text: print("Failed to find main load")
text = text.replace(old_main_load, new_main_load)

with open('src/c/main.c', 'w') as f:
    f.write(text)

print("Patch 2 Complete")
