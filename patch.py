import sys

with open('src/c/main.c', 'r') as f:
    text = f.read()

def safe_replace(old, new):
    global text
    if old not in text:
        print("Failed to find:\n" + old)
        sys.exit(1)
    text = text.replace(old, new)

safe_replace('static TextLayer *s_shutter_layer = NULL;\n', '')

safe_replace('''static AppTimer *s_marquee_timer = NULL; 
static AppTimer *s_shutter_timer = NULL;
static int s_shutter_countdown = 0;
static char s_shutter_buf[4] = "3";
static ActionBarLayer *s_action_bar = NULL;''', '''static AppTimer *s_marquee_timer = NULL; 
static ActionBarLayer *s_action_bar = NULL;''')

safe_replace('''static void shutter_timer_callback(void *context) {
    s_shutter_countdown--;
    if (s_shutter_countdown > 0) {
        snprintf(s_shutter_buf, sizeof(s_shutter_buf), "%d", s_shutter_countdown);
        text_layer_set_text(s_shutter_layer, s_shutter_buf);
        vibes_short_pulse(); // カウントダウンの鼓動
        s_shutter_timer = app_timer_register(1000, shutter_timer_callback, NULL);
    } else if (s_shutter_countdown == 0) {
        snprintf(s_shutter_buf, sizeof(s_shutter_buf), "0");
        text_layer_set_text(s_shutter_layer, s_shutter_buf);
        send_cmd(50); // シャッターコマンド送信
        vibes_long_pulse(); // 撮影完了の長い振動
        // 1秒後にUIを消すためのタイマー
        s_shutter_timer = app_timer_register(1000, shutter_timer_callback, NULL); 
    } else {
        // カウントダウン終了後、レイヤーを隠して元のUI状態に戻す
        layer_set_hidden(text_layer_get_layer(s_shutter_layer), true);
        s_shutter_timer = NULL;
    }
}

static void up_long_click_up_handler(ClickRecognizerRef recognizer, void *context) {
    if (s_app_state == 0 || s_app_state == 2 || s_app_state == 4 || s_app_state == 6) {
        s_shutter_countdown = 3;
        snprintf(s_shutter_buf, sizeof(s_shutter_buf), "%d", s_shutter_countdown);
        text_layer_set_text(s_shutter_layer, s_shutter_buf);
        text_layer_set_background_color(s_shutter_layer, s_current_main_bg);
        text_layer_set_text_color(s_shutter_layer, s_current_main_fg);
        layer_set_hidden(text_layer_get_layer(s_shutter_layer), false);
        vibes_short_pulse();
        if (s_shutter_timer) app_timer_cancel(s_shutter_timer);
        s_shutter_timer = app_timer_register(1000, shutter_timer_callback, NULL);
    }
}

static void click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
    window_long_click_subscribe(BUTTON_ID_UP, 2000, NULL, up_long_click_up_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
}''', '''static void click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
}''')

safe_replace('''    text_layer_set_background_color(s_clock_layer, GColorClear);
    text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
    text_layer_set_text(s_clock_layer, s_clock_buf);
    layer_add_child(wl, text_layer_get_layer(s_clock_layer));
    
    // シャッター用カウントダウンレイヤー（画面中央に帯状で配置、初期は非表示）
    s_shutter_layer = text_layer_create(GRect(0, (h / 2) - 30, wt, 60));
    text_layer_set_background_color(s_shutter_layer, GColorClear);
    text_layer_set_text_alignment(s_shutter_layer, GTextAlignmentCenter);
    text_layer_set_font(s_shutter_layer, fonts_get_system_font(FONT_HUGE_TIME));
    layer_set_hidden(text_layer_get_layer(s_shutter_layer), true);
    layer_add_child(wl, text_layer_get_layer(s_shutter_layer));

    load_persist_data();''', '''    text_layer_set_background_color(s_clock_layer, GColorClear);
    text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
    text_layer_set_text(s_clock_layer, s_clock_buf);
    layer_add_child(wl, text_layer_get_layer(s_clock_layer));

    load_persist_data();''')

safe_replace('''static void main_window_unload(Window *window) {
    destroy_marquee_layers();
    if (s_marquee_timer) {
        app_timer_cancel(s_marquee_timer);
        s_marquee_timer = NULL;
    }
    if (s_shutter_timer) {
        app_timer_cancel(s_shutter_timer);
        s_shutter_timer = NULL;
    }''', '''static void main_window_unload(Window *window) {
    destroy_marquee_layers();
    if (s_marquee_timer) {
        app_timer_cancel(s_marquee_timer);
        s_marquee_timer = NULL;
    }''')

safe_replace('''    if (s_step_layer) text_layer_destroy(s_step_layer);
    if (s_hr_layer) text_layer_destroy(s_hr_layer);
    if (s_shutter_layer) text_layer_destroy(s_shutter_layer);
    
    if (s_mid_bg_layer) layer_destroy(s_mid_bg_layer);''', '''    if (s_step_layer) text_layer_destroy(s_step_layer);
    if (s_hr_layer) text_layer_destroy(s_hr_layer);
    
    if (s_mid_bg_layer) layer_destroy(s_mid_bg_layer);''')

with open('src/c/main.c', 'w') as f:
    f.write(text)
print("Done")
