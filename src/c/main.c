#include <pebble.h>
#include "app_state.h"
#include "comm/graph_data.h"
#include "comm/comm_service.h"
#include "ui/ui_color_picker.h"
#include "ui/ui_intermediate_menu.h"
#include "ui/ui_activity_picker.h"
#include "ui/ui_marquee.h"
#include "ui/ui_graph_layer.h"

/* ==========================================================
   グローバル変数
   ========================================================== */
static Window *s_main_window;
static TextLayer *s_time_hour_layer, *s_time_colon1_layer, *s_time_min_layer, *s_time_colon2_layer, *s_time_sec_layer; 
static TextLayer *s_clock_layer, *s_dist_layer, *s_hr_layer, *s_step_layer;
static Layer *s_graph_layer, *s_mid_bg_layer;
static ActionBarLayer *s_action_bar = NULL;

static GFont s_font_huge_time;
static GFont s_font_long_time;
static GFont s_font_colon;
static GFont s_font_mid_data;

static GColor s_current_main_bg, s_current_main_fg;

static GBitmap *s_icon_play, *s_icon_pause, *s_icon_stop, *s_icon_check, *s_icon_trash, *s_icon_up, *s_icon_down, *s_icon_graph, *s_icon_save, *s_icon_setting;
static bool s_icons_loaded = false, s_current_icon_color_is_black = false;

static char s_time_hour_buf[8] = "0", s_time_min_buf[8] = "00", s_time_sec_buf[8] = "00";
static char s_clock_buf[16] = "--:--", s_dist_buf[16] = "--", s_hr_buf[16] = "--", s_step_buf[16] = "--";

static uint8_t s_app_state = 0; 
static bool s_is_paused = true;
static bool s_is_small_screen = false;
static bool s_is_round_screen = false;
static bool s_is_long_workout = false;
static bool s_has_hr_sensor = false; 

static int s_current_hr = 0;
static ActivityType s_current_activity = ACTIVITY_RUNNING;
static int s_selected_color_idx = 24; // GColorIslamicGreenARGB8

static GRect s_rect_hour_5, s_rect_col1_5, s_rect_min_5, s_rect_col2_5, s_rect_sec_5, s_rect_min_3, s_rect_col2_3, s_rect_sec_3;

static AppTimer *s_ignore_single_click_timer = NULL;
static bool s_ignore_single_click = false;
static uint64_t s_long_click_start_time = 0;

/* ==========================================================
   プロトタイプ宣言
   ========================================================== */
static void update_ui_state(void);
static void click_config_provider(void *context);
static void on_graph_dirty_request(void);

/* ==========================================================
   ユーティリティ・アイコン管理
   ========================================================== */
static void reset_ignore_single_click_callback(void *context) {
    s_ignore_single_click = false;
    s_ignore_single_click_timer = NULL;
}

static void trigger_ignore_single_click(void) {
    s_ignore_single_click = true;
    if (s_ignore_single_click_timer) app_timer_cancel(s_ignore_single_click_timer);
    s_ignore_single_click_timer = app_timer_register(600, reset_ignore_single_click_callback, NULL);
}

static void load_action_icons(bool is_black) {
    if (s_icons_loaded && s_current_icon_color_is_black == is_black) return;
    
    if (s_icons_loaded) {
        if (s_icon_play) gbitmap_destroy(s_icon_play);
        if (s_icon_pause) gbitmap_destroy(s_icon_pause);
        if (s_icon_stop) gbitmap_destroy(s_icon_stop);
        if (s_icon_check) gbitmap_destroy(s_icon_check);
        if (s_icon_trash) gbitmap_destroy(s_icon_trash);
        if (s_icon_up) gbitmap_destroy(s_icon_up);
        if (s_icon_down) gbitmap_destroy(s_icon_down);
        if (s_icon_graph) gbitmap_destroy(s_icon_graph);
        if (s_icon_save) gbitmap_destroy(s_icon_save);
        if (s_icon_setting) gbitmap_destroy(s_icon_setting);
    }
    
    if (is_black) {
        s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLAY_BLACK);
        s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PAUSE_BLACK);
        s_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STOP_BLACK);
        s_icon_check = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK_BLACK);
        s_icon_trash = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRASH_BLACK);
        s_icon_up = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP_BLACK);
        s_icon_down = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN_BLACK);
        s_icon_graph = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GRAPH_BLACK);
        s_icon_save = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SAVE_BLACK);
        s_icon_setting = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SETTING_BLACK);
    } else {
        s_icon_play = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLAY_WHITE);
        s_icon_pause = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PAUSE_WHITE);
        s_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STOP_WHITE);
        s_icon_check = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK_WHITE);
        s_icon_trash = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRASH_WHITE);
        s_icon_up = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP_WHITE);
        s_icon_down = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN_WHITE);
        s_icon_graph = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GRAPH_WHITE);
        s_icon_save = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SAVE_WHITE);
        s_icon_setting = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SETTING_WHITE);
    }
    
    s_current_icon_color_is_black = is_black;
    s_icons_loaded = true;
}

/* ==========================================================
   イベントハンドラ群 (Touch, Click)
   ========================================================== */
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
static void touch_event_handler(const TouchEvent *event, void *context) {
    uint64_t current_time = app_get_current_time_ms();
    static int s_touch_start_x = 0;
    static int s_touch_start_y = 0;
    static uint64_t s_touch_start_time = 0;
    static uint64_t s_last_tap_time = 0;

    switch (event->type) {
        case TouchEvent_Touchdown:
            s_touch_start_x = event->x;
            s_touch_start_y = event->y;
            s_touch_start_time = current_time;
            break;
        case TouchEvent_PositionUpdate:
            break;
        case TouchEvent_Liftoff:
            if (s_touch_start_time == 0) break;
            uint64_t dt = current_time - s_touch_start_time;
            int dx = event->x - s_touch_start_x;
            int dy = event->y - s_touch_start_y;
            int abs_dx = (dx > 0) ? dx : -dx;
            int abs_dy = (dy > 0) ? dy : -dy;

            if (dt < SWIPE_MAX_TIME_MS && (abs_dx > SWIPE_MIN_DIST_PX || abs_dy > SWIPE_MIN_DIST_PX)) {
                if (abs_dx > abs_dy * 2) {
                    if (dx > 0) comm_service_send_media_cmd(3); // PREV
                    else comm_service_send_media_cmd(2);        // NEXT
                } else if (abs_dy > abs_dx * 2) {
                    if (dy > 0) comm_service_send_media_cmd(5); // VOL DOWN
                    else comm_service_send_media_cmd(4);        // VOL UP
                }
                vibes_short_pulse();
                s_last_tap_time = 0; 
            } 
            else if (abs_dx <= TAP_MAX_DIST_PX && abs_dy <= TAP_MAX_DIST_PX) {
                if (s_last_tap_time != 0 && (current_time - s_last_tap_time) < DOUBLE_TAP_MAX_DELAY_MS) {
                    comm_service_send_media_cmd(1); // PLAY/PAUSE
                    vibes_short_pulse();
                    s_last_tap_time = 0; 
                } else {
                    s_last_tap_time = current_time;
                }
            }
            s_touch_start_time = 0;
            break;
    }
}
#endif

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
#if defined(PBL_COLOR)
    if (ui_color_picker_is_active() && s_action_bar) {
        int idx = (ui_color_picker_get_selected_idx() - 1 + 64) % 64;
        ui_color_picker_set_selected_idx(idx);
        update_ui_state();
        return;
    }
    if (ui_intermediate_menu_is_active() && s_action_bar) {
        ui_intermediate_menu_handle_down();
        return;
    }
#endif
    if (ui_activity_picker_is_active() && s_action_bar) {
        ui_activity_picker_handle_up();
        return;
    }
    if (s_ignore_single_click) return;

    if (s_app_state <= 4) {
        comm_service_send_cmd(1);
        vibes_short_pulse();
    } else if (s_app_state == 5) {
        comm_service_send_cmd(7);
        vibes_short_pulse();
    }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
#if defined(PBL_COLOR)
    if (ui_color_picker_is_active() && s_action_bar) {
        s_selected_color_idx = ui_color_picker_get_selected_idx();
        persist_write_int(PK_PERSONAL_COLOR, s_selected_color_idx);
        ui_color_picker_destroy();
        if (s_mid_bg_layer) layer_set_hidden(s_mid_bg_layer, false);
        if (s_graph_layer) layer_set_hidden(s_graph_layer, false);
        if (s_action_bar && s_main_window) {
            action_bar_layer_remove_from_window(s_action_bar);
            action_bar_layer_add_to_window(s_action_bar, s_main_window);
        }
        update_ui_state();
        vibes_short_pulse();
        return;
    }
#endif
    if (ui_activity_picker_is_active() && s_action_bar) {
        s_current_activity = ui_activity_picker_get_preview_activity();
        persist_write_int(PK_ACTIVITY_TYPE, s_current_activity);
        comm_service_send_activity_type(s_current_activity);
        ui_activity_picker_destroy();
        if (s_mid_bg_layer) layer_set_hidden(s_mid_bg_layer, false);
        if (s_graph_layer) layer_set_hidden(s_graph_layer, false);
        if (s_action_bar && s_main_window) {
            action_bar_layer_remove_from_window(s_action_bar);
            action_bar_layer_add_to_window(s_action_bar, s_main_window);
        }
        update_ui_state();
        vibes_short_pulse();
        return;
    }
#if defined(PBL_COLOR)
    if (ui_intermediate_menu_is_active() && s_action_bar) {
        int chosen = ui_intermediate_menu_get_selected_idx();
        ui_intermediate_menu_destroy();
        if (chosen == 0) {
            ui_activity_picker_create(s_main_window, s_action_bar, s_current_activity, s_current_main_bg, s_current_main_fg);
        } else {
            ui_color_picker_create(s_main_window, s_action_bar, s_current_main_bg, s_current_main_fg);
        }
        if (s_action_bar && s_main_window) {
            action_bar_layer_remove_from_window(s_action_bar);
            action_bar_layer_add_to_window(s_action_bar, s_main_window);
        }
        update_ui_state();
        vibes_short_pulse();
        return;
    }
#endif
    if (s_ignore_single_click) return;

    if (s_app_state == 3) {
        int mid_count = graph_data_get_mid_page_count();
        if (mid_count > 0) {
            int current_mode = graph_data_get_current_mid_mode();
            current_mode = (current_mode + 1) % mid_count;
            graph_data_set_current_mid_mode(current_mode);
            
            // 新規: 選択した中段項目のIDをスマホへ通知
            const MidPageData *page = graph_data_get_current_mid_page();
            if (page) {
                comm_service_send_mid_id(page->id);
            }

            update_ui_state();
            vibes_short_pulse();
        }
        return;
    }

    if (s_app_state <= 2) {
#if defined(PBL_COLOR)
        ui_intermediate_menu_create(s_main_window, s_action_bar, s_current_main_bg, s_current_main_fg);
#else
        ui_activity_picker_create(s_main_window, s_action_bar, s_current_activity, s_current_main_bg, s_current_main_fg);
#endif
        if (s_mid_bg_layer) layer_set_hidden(s_mid_bg_layer, true);
        if (s_graph_layer) layer_set_hidden(s_graph_layer, true);
        if (s_action_bar && s_main_window) {
            action_bar_layer_remove_from_window(s_action_bar);
            action_bar_layer_add_to_window(s_action_bar, s_main_window);
        }
        update_ui_state();
        vibes_short_pulse();
    } else if (s_app_state == 4) {
        comm_service_send_cmd(2);
        vibes_short_pulse();
    } else if (s_app_state == 6) {
        comm_service_send_cmd(9);
        vibes_short_pulse();
    }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
#if defined(PBL_COLOR)
    if (ui_color_picker_is_active() && s_action_bar) {
        ui_color_picker_handle_down();
        update_ui_state();
        return;
    }
    if (ui_intermediate_menu_is_active() && s_action_bar) {
        ui_intermediate_menu_handle_down();
        return;
    }
#endif
    if (ui_activity_picker_is_active() && s_action_bar) {
        ui_activity_picker_handle_down();
        return;
    }
    if (s_ignore_single_click) return;

    if (s_app_state == 3) {
        int lower_count = graph_data_get_lower_page_count();
        if (lower_count > 0) {
            // 下段リストが設定されている場合はカルーセル切り替え
            int current_mode = graph_data_get_current_lower_mode();
            current_mode = (current_mode + 1) % lower_count;
            graph_data_set_current_lower_mode(current_mode);
            
            const LowerPageData *page = graph_data_get_current_lower_page();
            if (page) {
                comm_service_send_lower_id(page->id);
                // グラフ項目の場合は互換のためCMD:6も送信
                if (page->id >= 100) {
                    comm_service_send_cmd(6);
                }
            }
            update_ui_state();
            vibes_short_pulse();
        } else {
            // 従来通りGraphSync (CMD: 6) を送信
            comm_service_send_cmd(6);
            vibes_short_pulse();
        }
        return;
    }

    if (s_app_state != 5) {
        comm_service_send_cmd(6);
        vibes_short_pulse();
    } else if (s_app_state == 5) {
        comm_service_send_cmd(8);
        vibes_short_pulse();
    }
}

static void generic_long_click_down_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
    ignore = ignore || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    s_long_click_start_time = app_get_current_time_ms();
    vibes_short_pulse();
}

static void up_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
    ignore = ignore || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    
    if (app_get_current_time_ms() - s_long_click_start_time >= 1200) return;

    comm_service_send_cmd(50);
    ui_marquee_trigger_custom("UP LONG SEND", s_current_main_fg, s_current_main_bg, s_app_state);
    vibes_short_pulse();
}

static void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
    ignore = ignore || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    
    if (app_get_current_time_ms() - s_long_click_start_time >= 1200) return;

    comm_service_send_cmd(51);
    ui_marquee_trigger_custom("SELECT LONG SEND", s_current_main_fg, s_current_main_bg, s_app_state);
    vibes_short_pulse();
}

static void down_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
    ignore = ignore || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    
    if (app_get_current_time_ms() - s_long_click_start_time >= 1200) return;

    comm_service_send_cmd(52);
    ui_marquee_trigger_custom("DOWN LONG SEND", s_current_main_fg, s_current_main_bg, s_app_state);
    vibes_short_pulse();
}

static void click_config_provider(void *context) {
    bool custom_clicks = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
    custom_clicks = custom_clicks || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
    if (custom_clicks) {
        window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
        window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
        window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
        return;
    }

    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_long_click_subscribe(BUTTON_ID_UP, 800, generic_long_click_down_handler, up_long_click_release_handler);

    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_long_click_subscribe(BUTTON_ID_SELECT, 800, generic_long_click_down_handler, select_long_click_release_handler);

    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_long_click_subscribe(BUTTON_ID_DOWN, 800, generic_long_click_down_handler, down_long_click_release_handler);
}

/* ==========================================================
   描画コールバック関数
   ========================================================== */
static void graph_layer_update_callback(Layer *layer, GContext *ctx) {
    ui_graph_layer_update_proc(layer, ctx, s_app_state, s_current_main_fg);
}

static void mid_bg_layer_update_callback(Layer *layer, GContext *ctx) {
    ui_mid_bg_layer_update_proc(layer, ctx, s_app_state, s_current_main_bg, s_current_main_fg, s_is_paused, s_current_hr);
}

static void on_graph_dirty_request(void) {
    if (s_graph_layer) layer_mark_dirty(s_graph_layer);
}

/* ==========================================================
   UI状態更新関数
   ========================================================== */
static void update_ui_state(void) {
    GColor pc = GColorWhite;
#if defined(PBL_COLOR)
    pc = (GColor){.argb = ui_color_picker_get_personal_color_argb()};
#endif

    bool is_active = (s_app_state == 3);

#if defined(PBL_COLOR)
    bool pc_is_bright = gcolor_equal(gcolor_legible_over(pc), GColorBlack);

    if (ui_color_picker_is_active() && s_action_bar) {
        s_current_main_bg = ui_color_picker_get_preview_is_running() ? pc : gcolor_legible_over(pc);
        s_current_main_fg = ui_color_picker_get_preview_is_running() ? gcolor_legible_over(pc) : pc;
    } else {
        if (is_active) {
            s_current_main_bg = pc_is_bright ? pc : GColorWhite;
            s_current_main_fg = pc_is_bright ? GColorBlack : pc;
        } else {
            s_current_main_bg = pc_is_bright ? GColorBlack : pc;
            s_current_main_fg = pc_is_bright ? pc : GColorWhite;
        }
    }
#else
    if (is_active) {
        s_current_main_bg = GColorWhite;
        s_current_main_fg = GColorBlack;
    } else {
        s_current_main_bg = GColorBlack;
        s_current_main_fg = GColorWhite;
    }
#endif

    if (s_main_window) {
        window_set_background_color(s_main_window, s_current_main_bg);
    }
    
    bool hide = false;
    bool hide_cond = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
    hide_cond = hide_cond || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
    if (s_action_bar && hide_cond) hide = true;

    bool hide_mid = hide || !graph_data_is_detail_mode(s_app_state);

    if (s_dist_layer) layer_set_hidden(text_layer_get_layer(s_dist_layer), hide_mid);
    if (s_step_layer) layer_set_hidden(text_layer_get_layer(s_step_layer), hide_mid);
    
    if (!s_has_hr_sensor) {
        if (s_hr_layer) layer_set_hidden(text_layer_get_layer(s_hr_layer), true);
    } else {
        if (s_hr_layer) layer_set_hidden(text_layer_get_layer(s_hr_layer), hide_mid);
    }
    
    if (s_clock_layer) layer_set_hidden(text_layer_get_layer(s_clock_layer), hide_mid);

    if ((s_app_state < 3 || s_app_state >= 5) && !hide) {
        ui_marquee_trigger(s_app_state, s_current_main_fg, s_current_main_bg);
    } else {
        ui_marquee_destroy();
    }

    if (s_is_long_workout) {
#if !defined(PBL_PLATFORM_APLITE)
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_GABBRO)
        text_layer_set_font(s_time_hour_layer, s_font_long_time);
        text_layer_set_font(s_time_min_layer, s_font_huge_time);
        text_layer_set_font(s_time_sec_layer, s_font_huge_time);
#else
        text_layer_set_font(s_time_hour_layer, s_font_long_time);
        text_layer_set_font(s_time_min_layer, s_font_long_time);
        text_layer_set_font(s_time_sec_layer, s_font_long_time);
#endif
#endif
        layer_set_frame(text_layer_get_layer(s_time_hour_layer), s_rect_hour_5);
        layer_set_frame(text_layer_get_layer(s_time_colon1_layer), s_rect_col1_5);
        layer_set_frame(text_layer_get_layer(s_time_min_layer), s_rect_min_5);
        layer_set_frame(text_layer_get_layer(s_time_colon2_layer), s_rect_col2_5);
        layer_set_frame(text_layer_get_layer(s_time_sec_layer), s_rect_sec_5);
    } else {
#if !defined(PBL_PLATFORM_APLITE)
        text_layer_set_font(s_time_min_layer, s_font_huge_time);
        text_layer_set_font(s_time_sec_layer, s_font_huge_time);
#endif
        layer_set_frame(text_layer_get_layer(s_time_min_layer), s_rect_min_3);
        layer_set_frame(text_layer_get_layer(s_time_colon2_layer), s_rect_col2_3);
        layer_set_frame(text_layer_get_layer(s_time_sec_layer), s_rect_sec_3);
    }
    
    if (s_time_hour_layer) layer_set_hidden(text_layer_get_layer(s_time_hour_layer), !s_is_long_workout);
    if (s_time_colon1_layer) layer_set_hidden(text_layer_get_layer(s_time_colon1_layer), !s_is_long_workout);
    
    if (s_time_hour_layer) text_layer_set_text_color(s_time_hour_layer, s_current_main_fg);
    if (s_time_colon1_layer) text_layer_set_text_color(s_time_colon1_layer, s_current_main_fg);
    if (s_time_min_layer) text_layer_set_text_color(s_time_min_layer, s_current_main_fg);
    if (s_time_colon2_layer) text_layer_set_text_color(s_time_colon2_layer, s_current_main_fg);
    if (s_time_sec_layer) text_layer_set_text_color(s_time_sec_layer, s_current_main_fg);
    
    GColor mid_text_color = is_active ? s_current_main_bg : s_current_main_fg;
    if (s_clock_layer) text_layer_set_text_color(s_clock_layer, mid_text_color);
    if (s_dist_layer) text_layer_set_text_color(s_dist_layer, mid_text_color);
    if (s_hr_layer) text_layer_set_text_color(s_hr_layer, mid_text_color);
    if (s_step_layer) text_layer_set_text_color(s_step_layer, mid_text_color);

    if (s_action_bar) {
        action_bar_layer_set_background_color(s_action_bar, pc);
        
        bool use_black_icons = gcolor_equal(gcolor_legible_over(pc), GColorBlack);
        load_action_icons(use_black_icons);
        
        bool icon_cond = ui_activity_picker_is_active();
#if defined(PBL_COLOR)
        icon_cond = icon_cond || ui_color_picker_is_active() || ui_intermediate_menu_is_active();
#endif
        if (icon_cond) {
            action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_up);
#if defined(PBL_COLOR)
            if (ui_intermediate_menu_is_active()) {
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_check);
            } else {
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_save);
            }
#else
            action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_save);
#endif
            action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_down);
        } else {
            if (s_app_state < 3) {
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_play);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_setting);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_graph);
            } else if (s_app_state == 3) { 
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_pause);
                action_bar_layer_clear_icon(s_action_bar, BUTTON_ID_SELECT);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_graph);
            } else if (s_app_state == 4) { 
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_play);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_stop);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_graph);
            } else if (s_app_state == 5) { 
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_save);
                action_bar_layer_clear_icon(s_action_bar, BUTTON_ID_SELECT);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_trash);
            } else if (s_app_state == 6) { 
                action_bar_layer_clear_icon(s_action_bar, BUTTON_ID_UP);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_check);
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_graph);
            }
        }
    }
    
    if (s_mid_bg_layer && s_action_bar) {
        if (is_active) {
            layer_insert_above_sibling(s_mid_bg_layer, action_bar_layer_get_layer(s_action_bar));
        } else {
            layer_insert_below_sibling(s_mid_bg_layer, action_bar_layer_get_layer(s_action_bar));
        }
        
        if (s_graph_layer) layer_insert_above_sibling(s_graph_layer, s_mid_bg_layer);
        if (s_time_hour_layer) layer_insert_above_sibling(text_layer_get_layer(s_time_hour_layer), s_mid_bg_layer);
        if (s_time_colon1_layer) layer_insert_above_sibling(text_layer_get_layer(s_time_colon1_layer), s_mid_bg_layer);
        if (s_time_min_layer) layer_insert_above_sibling(text_layer_get_layer(s_time_min_layer), s_mid_bg_layer);
        if (s_time_colon2_layer) layer_insert_above_sibling(text_layer_get_layer(s_time_colon2_layer), s_mid_bg_layer);
        if (s_time_sec_layer) layer_insert_above_sibling(text_layer_get_layer(s_time_sec_layer), s_mid_bg_layer);
        if (s_dist_layer) layer_insert_above_sibling(text_layer_get_layer(s_dist_layer), s_mid_bg_layer);
        if (s_step_layer) layer_insert_above_sibling(text_layer_get_layer(s_step_layer), s_mid_bg_layer);
        if (s_hr_layer) layer_insert_above_sibling(text_layer_get_layer(s_hr_layer), s_mid_bg_layer);
        if (s_clock_layer) layer_insert_above_sibling(text_layer_get_layer(s_clock_layer), s_mid_bg_layer);
    }

    if (s_mid_bg_layer) layer_mark_dirty(s_mid_bg_layer);
    if (s_graph_layer) layer_mark_dirty(s_graph_layer);

#if defined(PBL_COLOR)
    if (ui_color_picker_is_active()) ui_color_picker_update_colors(s_current_main_bg, s_current_main_fg);
    if (ui_intermediate_menu_is_active()) ui_intermediate_menu_update_colors(s_current_main_bg, s_current_main_fg);
#endif
    if (ui_activity_picker_is_active()) ui_activity_picker_update_colors(s_current_main_bg, s_current_main_fg);
}

/* ==========================================================
   タイマーハンドラ
   ========================================================== */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    static char last_clock_buf[16] = "";
    clock_copy_time_string(s_clock_buf, 16);
    if (strcmp(s_clock_buf, last_clock_buf) != 0) {
        strcpy(last_clock_buf, s_clock_buf);
        if (s_clock_layer) {
            text_layer_set_text(s_clock_layer, s_clock_buf);
        }
    }
    
    static int state0_demo_timer = 0;
    if (s_app_state == 0) {
        state0_demo_timer++;
        if (state0_demo_timer >= 3) {
            s_is_long_workout = !s_is_long_workout;
            if (s_is_long_workout) {
                snprintf(s_time_hour_buf, 8, "0");
                snprintf(s_time_min_buf, 8, "00");
                snprintf(s_time_sec_buf, 8, "00");
            } else {
                snprintf(s_time_min_buf, 8, "00");
                snprintf(s_time_sec_buf, 8, "00");
            }
            update_ui_state();
            state0_demo_timer = 0;
        }
    }

#if defined(PBL_HEALTH)
    static int last_hr_display = -1;
    if (s_has_hr_sensor) {
        int32_t interval = comm_service_get_hr_interval_setting();
        HealthMetric metric_to_peek = (interval > 0) ? HealthMetricHeartRateRawBPM : HealthMetricHeartRateBPM;
        s_current_hr = (int)health_service_peek_current_value(metric_to_peek);
        if (s_current_hr != last_hr_display) {
            last_hr_display = s_current_hr;
            if (s_current_hr < 30) snprintf(s_hr_buf, 16, "--");
            else snprintf(s_hr_buf, 16, "%d", s_current_hr);
            if (s_hr_layer) text_layer_set_text(s_hr_layer, s_hr_buf);
        }
    } else {
        if (last_hr_display != -2) {
            snprintf(s_hr_buf, 16, "--");
            last_hr_display = -2;
        }
    }
#else
    snprintf(s_hr_buf, 16, "--");
#endif
    
#if defined(PBL_COLOR)
    if (ui_color_picker_is_active() && s_action_bar) {
        ui_color_picker_toggle_preview();
        update_ui_state();
    }
#endif

    static int s_seconds_counter = 0;
#if defined(PBL_HEALTH)
    static int last_sent_hr = 0;
    static int hr_no_change_counter = 0;
    static uint32_t s_current_sampling_period = 9999;
#endif
    static uint8_t last_tick_app_state = 0;

    if (s_app_state == 3) {
        if (last_tick_app_state != 3) {
            s_seconds_counter = 0;
#if defined(PBL_HEALTH)
            last_sent_hr = 0;
            hr_no_change_counter = 0;
            s_current_sampling_period = 9999;
#endif
        }
        s_seconds_counter++;

#if defined(PBL_HEALTH)
        if (s_has_hr_sensor) {
            bool should_send_steps = (s_seconds_counter % 5 == 0);
            bool should_send_hr = false;
            uint32_t target_sample_period = 0;

            int32_t interval = comm_service_get_hr_interval_setting();
            const int BURST_APPLIES_ABOVE_INTERVAL = 10;
            bool burst_mode_applies = (interval > BURST_APPLIES_ABOVE_INTERVAL);

            if (burst_mode_applies) {
                const int BURST_TOTAL_DURATION = 10;
                const int BURST_WARMUP_DURATION = 5;
                int seconds_into_cycle = (s_seconds_counter - 1) % interval;
                bool is_in_burst_window = (seconds_into_cycle < BURST_TOTAL_DURATION);

                if (is_in_burst_window) {
                    target_sample_period = 1;
                    if (seconds_into_cycle >= BURST_WARMUP_DURATION) {
                        should_send_hr = true;
                    }
                } else {
                    target_sample_period = 0;
                }
            } else {
                if (interval < 0) {
                    target_sample_period = abs(interval);
                } else if (interval > 0) {
                    target_sample_period = 1;
                } else {
                    target_sample_period = 0;
                }

                bool hr_value_changed = (s_current_hr != last_sent_hr);
                if (!hr_value_changed) hr_no_change_counter++;
                should_send_hr = hr_value_changed || (hr_no_change_counter >= 5);
            }

            if (target_sample_period != s_current_sampling_period) {
                health_service_set_heart_rate_sample_period(target_sample_period);
                s_current_sampling_period = target_sample_period;
            }

            int step_val = (int)health_service_sum_today(HealthMetricStepCount);
            if (should_send_hr) {
                last_sent_hr = s_current_hr;
                hr_no_change_counter = 0;
            }
            comm_service_send_health_data(should_send_steps, should_send_hr, step_val, s_current_hr);
        }
#endif
    } else {
        if (last_tick_app_state == 3) {
#if defined(PBL_HEALTH)
            if (s_current_sampling_period != 0) {
                health_service_set_heart_rate_sample_period(0);
                s_current_sampling_period = 0;
            }
#endif
        }
    }
    last_tick_app_state = s_app_state;
}

/* ==========================================================
   ウィンドウロード・メイン関数
   ========================================================== */
static void main_window_load(Window *window) {
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
    int wt = b.size.w;
    int h = b.size.h;

    s_is_small_screen = (wt <= 144 && h <= 168);
#if defined(PBL_ROUND)
    s_is_round_screen = true;
#else
    s_is_round_screen = (wt == 180 && h == 180) || (wt == 260 && h == 260); 
#endif

#if defined(PBL_HEALTH)
    s_has_hr_sensor = (health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL)) & HealthServiceAccessibilityMaskAvailable) != 0;
#else
    s_has_hr_sensor = false;
#endif

    s_action_bar = action_bar_layer_create();
    action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
    action_bar_layer_add_to_window(s_action_bar, window);

    s_mid_bg_layer = layer_create(GRect(0, 0, wt, h));
    layer_set_update_proc(s_mid_bg_layer, mid_bg_layer_update_callback);
    layer_add_child(wl, s_mid_bg_layer);

    int active_w = wt - ACTION_BAR_WIDTH;

    int m3w = 0, c3w = 0, s3w = 0;
    int h5w = 0, c1w = 0, m5w = 0, c2w = 0, s5w = 0;
    int r_h3 = 0, r_h5 = 0;
    int y3_base_m = 0, y3_base_s = 0, y3_colon = 0;
    int y5_base_h = 0, y5_base_m = 0, y5_base_s = 0, y5_colon1 = 0, y5_colon2 = 0;
    int colon_x_offset = 0, offset_x3 = 0, offset_x5 = 0;
    bool use_overlap = false;
    int vm3 = 0, vc3 = 0, vs3 = 0;
    int vh5 = 0, vc1 = 0, vm5 = 0, vc2 = 0, vs5 = 0;
    int upper_h = 0, mid_h = 0, lower_h = 0;
    int row1_y = 0, row2_y = 0;
    int lx = 0, rx = 0, row_h = 0;

#if defined(PBL_PLATFORM_APLITE)
    s_font_huge_time = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
    s_font_long_time = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
    s_font_colon = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    s_font_mid_data = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    
    use_overlap = false;
    m3w = 42; c3w = 10; s3w = 42;
    h5w = 22; c1w = 6; m5w = 42; c2w = 6; s5w = 42;
    r_h3 = 40; r_h5 = 40;
    
    int h3 = h / 3;
    y3_base_m = (h3 / 2) - 18;
    y3_base_s = y3_base_m;
    y3_colon = y3_base_m + 6;
    y5_base_h = y3_base_m;
    y5_base_m = y3_base_m;
    y5_base_s = y3_base_m;
    y5_colon1 = y5_base_m + 6;
    y5_colon2 = y5_base_m + 6;
    colon_x_offset = 0;
    
    mid_h = h3; upper_h = h3; lower_h = h - mid_h - upper_h;
    row_h = 28;
    lx = 18; rx = (active_w / 2) + 20;
    row1_y = upper_h + 5; row2_y = upper_h + 33;

#elif defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT)
    s_font_huge_time = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    s_font_long_time = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
    s_font_colon = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    s_font_mid_data = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    
    use_overlap = false;
    m3w = 52; c3w = 8; s3w = 52;
    h5w = 22; c1w = 6; m5w = 42; c2w = 6; s5w = 42;
    r_h3 = 50; r_h5 = 50;
    
    int h3 = h / 3;
    y3_base_m = (h3 / 2) - 20;
    y3_base_s = y3_base_m;
    y3_colon = y3_base_m + 14;
    y5_base_h = y3_base_m + 4;
    y5_base_m = y3_base_m + 4;
    y5_base_s = y3_base_m + 4;
    y5_colon1 = y5_base_h + 8;
    y5_colon2 = y5_base_h + 8;
    colon_x_offset = -2;
    
    mid_h = h3; upper_h = h3; lower_h = h - mid_h - upper_h;
    row_h = 28;
    lx = 18; rx = (active_w / 2) + 20;
    row1_y = upper_h + 5; row2_y = upper_h + 33;

#elif defined(PBL_PLATFORM_CHALK)
    s_font_huge_time = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    s_font_long_time = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
    s_font_colon = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    s_font_mid_data = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    
    use_overlap = true;
    m3w = 60; c3w = 14; s3w = 60;
    vm3 = 52; vc3 = 6; vs3 = 52;
    
    h5w = 26; c1w = 12; m5w = 60; c2w = 14; s5w = 60;
    vh5 = 28; vc1 = 0; vm5 = 52; vc2 = 6; vs5 = 52;
    
    r_h3 = 50; r_h5 = 50;
    mid_h = 45; upper_h = (h - mid_h) / 2; lower_h = h - mid_h - upper_h;
    
    y3_base_m = upper_h - 48; 
    y3_base_s = y3_base_m;
    y3_colon = y3_base_m + 12;
    
    y5_base_m = upper_h - 48; 
    y5_base_h = upper_h - 26;
    y5_base_s = upper_h - 48; 
    y5_colon1 = y5_base_h + 2;
    y5_colon2 = y5_base_m + 12;
    offset_x3 = 11;
    offset_x5 = -3;
    
    row_h = 18;
    int mx = 20;
    lx = mx - 4 + 8; 
    rx = (active_w / 2) + 16 + 8;
    row1_y = upper_h + (mid_h - row_h * 2) / 2 + 2;
    row2_y = row1_y + row_h;

#elif defined(PBL_PLATFORM_EMERY)
    s_font_huge_time = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
    s_font_long_time = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    s_font_colon = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    s_font_mid_data = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    
    use_overlap = true;
    m3w = 80; c3w = 16; s3w = 80;
    vm3 = 68; vc3 = 8; vs3 = 68;
    
    h5w = 48; c1w = 14; m5w = 60; c2w = 14; s5w = 60;
    vh5 = 42; vc1 = 6; vm5 = 54; vc2 = 6; vs5 = 54;
    r_h3 = 68; r_h5 = 60;
    
    int h3 = h / 3;
    y3_base_m = (h3 / 2) - 36;
    y3_base_s = (h3 / 2) - 36;
    y3_colon = y3_base_m + 22;
    y5_base_h = (h3 / 2) - 26;
    y5_base_m = (h3 / 2) - 26;
    y5_base_s = (h3 / 2) - 26;
    y5_colon1 = y5_base_h + 12;
    y5_colon2 = y5_base_h + 12;
    
    offset_x3 = 0;
    offset_x5 = -6;
    
    mid_h = h3; upper_h = h3; lower_h = h - mid_h - upper_h;
    row_h = 28;
    lx = 32; rx = (active_w / 2) + 26;
    row1_y = upper_h + 5; row2_y = upper_h + 33;

#elif defined(PBL_PLATFORM_GABBRO)
    s_font_huge_time = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
    s_font_long_time = fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS);
    s_font_colon = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    s_font_mid_data = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    
    use_overlap = true;
    m3w = 80; c3w = 16; s3w = 80;
    vm3 = 72; vc3 = 10; vs3 = 72;
    
    h5w = 48; c1w = 16; m5w = 80; c2w = 16; s5w = 80;
    vh5 = 52; vc1 = 2; vm5 = 72; vc2 = 10; vs5 = 72;
    
    r_h3 = 72; r_h5 = 72;
    mid_h = 65; upper_h = (h - mid_h) / 2; lower_h = h - mid_h - upper_h;
    
    y3_base_m = upper_h - 68; 
    y3_base_s = y3_base_m;
    y3_colon = y3_base_m + 22;
    
    y5_base_m = upper_h - 68; 
    y5_base_h = upper_h - 46;
    y5_base_s = upper_h - 68; 
    y5_colon1 = y5_base_h + 10;
    y5_colon2 = y5_base_m + 22;
    offset_x3 = 15;
    offset_x5 = -12;
    
    row_h = 28;
    int mx = 30;
    lx = mx + 10 + 8;
    rx = (active_w / 2) + 22 + 8;
    row1_y = upper_h + (mid_h - row_h * 2) / 2 - 2;
    row2_y = row1_y + row_h;
#else
    s_font_huge_time = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
    s_font_long_time = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
    s_font_colon = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    s_font_mid_data = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    
    use_overlap = false;
    m3w = 60; c3w = 12; s3w = 60;
    h5w = 30; c1w = 12; m5w = 60; c2w = 12; s5w = 60;
    r_h3 = 50; r_h5 = 50;
    
    int h3 = h / 3;
    y3_base_m = (h3 / 2) - 25;
    y3_base_s = y3_base_m;
    y3_colon = y3_base_m + 10;
    y5_base_h = y3_base_m + 4;
    y5_base_m = y3_base_m + 4;
    y5_base_s = y3_base_m + 4;
    y5_colon1 = y5_base_h + 8;
    y5_colon2 = y5_base_h + 8;
    
    mid_h = h3; upper_h = h3; lower_h = h - mid_h - upper_h;
    row_h = 28;
    lx = 26; rx = (active_w / 2) + 20;
    row1_y = upper_h + 5; row2_y = upper_h + 33;
#endif

    s_graph_layer = layer_create(GRect(0, upper_h + mid_h, active_w, lower_h));
    layer_set_update_proc(s_graph_layer, graph_layer_update_callback);
    layer_add_child(wl, s_graph_layer);

    ui_marquee_init(window, s_action_bar, s_graph_layer);

    if (use_overlap) {
        int t3 = vm3 + vc3 + vs3;
        int sx3 = (active_w - t3) / 2 + offset_x3;
        int cx_m3 = sx3 + vm3 / 2;
        int cx_c3 = sx3 + vm3 + vc3 / 2;
        int cx_s3 = sx3 + vm3 + vc3 + vs3 / 2;
        
        int t5 = vh5 + vc1 + vm5 + vc2 + vs5;
        int sx5 = (active_w - t5) / 2 + offset_x5;
        int cx_h5 = sx5 + vh5 / 2;
        int cx_c1 = sx5 + vh5 + vc1 / 2;
        int cx_m5 = sx5 + vh5 + vc1 + vm5 / 2;
        int cx_c2 = sx5 + vh5 + vc1 + vm5 + vc2 / 2;
        int cx_s5 = sx5 + vh5 + vc1 + vm5 + vc2 + vs5 / 2;

        s_rect_hour_5 = GRect(cx_h5 - h5w/2, y5_base_h, h5w, r_h5);
        s_rect_col1_5 = GRect(cx_c1 - c1w/2 + colon_x_offset, y5_colon1, c1w, r_h5);
        s_rect_min_5 = GRect(cx_m5 - m5w/2, y5_base_m, m5w, r_h5);
        s_rect_col2_5 = GRect(cx_c2 - c2w/2 + colon_x_offset, y5_colon2, c2w, r_h5);
        s_rect_sec_5 = GRect(cx_s5 - s5w/2, y5_base_s, s5w, r_h5);
        
        s_rect_min_3 = GRect(cx_m3 - m3w/2, y3_base_m, m3w, r_h3);
        s_rect_col2_3 = GRect(cx_c3 - c3w/2 + colon_x_offset, y3_colon, c3w, r_h3);
        s_rect_sec_3 = GRect(cx_s3 - s3w/2, y3_base_s, s3w, r_h3);
    } else {
        int t3 = m3w + c3w + s3w;
        int sx3 = (active_w - t3) / 2 + offset_x3;
        s_rect_min_3 = GRect(sx3, y3_base_m, m3w, r_h3);
        s_rect_col2_3 = GRect(sx3 + m3w + colon_x_offset, y3_colon, c3w, r_h3);
        s_rect_sec_3 = GRect(sx3 + m3w + c3w, y3_base_s, s3w, r_h3);
        
        int t5 = h5w + c1w + m5w + c2w + s5w;
        int sx5 = (active_w - t5) / 2 + offset_x5;
        s_rect_hour_5 = GRect(sx5, y5_base_h, h5w, r_h5);
        s_rect_col1_5 = GRect(sx5 + h5w + colon_x_offset, y5_colon1, c1w, r_h5);
        s_rect_min_5 = GRect(sx5 + h5w + c1w, y5_base_m, m5w, r_h5);
        s_rect_col2_5 = GRect(sx5 + h5w + c1w + m5w + colon_x_offset, y5_colon2, c2w, r_h5);
        s_rect_sec_5 = GRect(sx5 + h5w + c1w + m5w + c2w, y5_base_s, s5w, r_h5);
    }

    int tw_left = rx - lx - 2;
    int tw_right = active_w - rx;

    s_dist_layer = text_layer_create(GRect(lx, row1_y, tw_left, row_h));
    s_step_layer = text_layer_create(GRect(rx, row1_y, tw_right, row_h));
    s_hr_layer = text_layer_create(GRect(lx, row2_y, tw_left, row_h));
    s_clock_layer = text_layer_create(GRect(rx, row2_y, tw_right, row_h));
    
    s_time_hour_layer = text_layer_create(s_rect_hour_5);
    text_layer_set_background_color(s_time_hour_layer, GColorClear);
    text_layer_set_text_alignment(s_time_hour_layer, GTextAlignmentRight);
    text_layer_set_text(s_time_hour_layer, s_time_hour_buf);
    text_layer_set_font(s_time_hour_layer, s_font_long_time);
    layer_add_child(wl, text_layer_get_layer(s_time_hour_layer));
    
    s_time_colon1_layer = text_layer_create(s_rect_col1_5);
    text_layer_set_background_color(s_time_colon1_layer, GColorClear);
    text_layer_set_text_alignment(s_time_colon1_layer, GTextAlignmentCenter);
#if defined(PBL_PLATFORM_CHALK)
    text_layer_set_font(s_time_colon1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
#elif defined(PBL_PLATFORM_GABBRO)
    text_layer_set_font(s_time_colon1_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
#else
    text_layer_set_font(s_time_colon1_layer, s_font_colon);
#endif
    text_layer_set_text(s_time_colon1_layer, ":");
    layer_add_child(wl, text_layer_get_layer(s_time_colon1_layer));
    
    s_time_min_layer = text_layer_create(s_rect_min_3);
    text_layer_set_background_color(s_time_min_layer, GColorClear);
    text_layer_set_text_alignment(s_time_min_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_min_layer, s_font_huge_time);
    text_layer_set_text(s_time_min_layer, s_time_min_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_min_layer));
    
    s_time_colon2_layer = text_layer_create(s_rect_col2_3);
    text_layer_set_background_color(s_time_colon2_layer, GColorClear);
    text_layer_set_text_alignment(s_time_colon2_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_colon2_layer, s_font_colon);
    text_layer_set_text(s_time_colon2_layer, ":");
    layer_add_child(wl, text_layer_get_layer(s_time_colon2_layer));
    
    s_time_sec_layer = text_layer_create(s_rect_sec_3);
    text_layer_set_background_color(s_time_sec_layer, GColorClear);
    text_layer_set_text_alignment(s_time_sec_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_sec_layer, s_font_huge_time);
    text_layer_set_text(s_time_sec_layer, s_time_sec_buf);
    layer_add_child(wl, text_layer_get_layer(s_time_sec_layer));
    
    text_layer_set_font(s_dist_layer, s_font_mid_data);
    text_layer_set_background_color(s_dist_layer, GColorClear);
    text_layer_set_text_alignment(s_dist_layer, GTextAlignmentLeft);
    text_layer_set_text(s_dist_layer, s_dist_buf);
    layer_add_child(wl, text_layer_get_layer(s_dist_layer));
    
    text_layer_set_font(s_step_layer, s_font_mid_data);
    text_layer_set_background_color(s_step_layer, GColorClear);
    text_layer_set_text_alignment(s_step_layer, GTextAlignmentLeft);
    text_layer_set_text(s_step_layer, s_step_buf);
    layer_add_child(wl, text_layer_get_layer(s_step_layer));
    
    text_layer_set_font(s_hr_layer, s_font_mid_data);
    text_layer_set_background_color(s_hr_layer, GColorClear);
    text_layer_set_text_alignment(s_hr_layer, GTextAlignmentLeft);
    text_layer_set_text(s_hr_layer, s_hr_buf);
    layer_add_child(wl, text_layer_get_layer(s_hr_layer));
    
    text_layer_set_font(s_clock_layer, s_font_mid_data);
    text_layer_set_background_color(s_clock_layer, GColorClear);
    text_layer_set_text_alignment(s_clock_layer, GTextAlignmentLeft);
    text_layer_set_text(s_clock_layer, s_clock_buf);
    layer_add_child(wl, text_layer_get_layer(s_clock_layer));

    graph_data_load_from_persist(&s_current_activity, &s_selected_color_idx);
#if defined(PBL_COLOR)
    ui_color_picker_set_selected_idx(s_selected_color_idx);
#endif

    comm_service_set_ui_buffers(
        s_time_hour_buf, s_time_min_buf, s_time_sec_buf,
        s_dist_buf, s_hr_buf, s_step_buf,
        s_dist_layer, s_hr_layer, s_step_layer,
        &s_app_state, &s_is_paused, &s_is_long_workout,
        &s_current_activity, s_has_hr_sensor
    );

    update_ui_state(); 
    tick_handler(NULL, SECOND_UNIT); 
}

static void main_window_unload(Window *window) {
    ui_marquee_destroy();
    
    if (s_ignore_single_click_timer) {
        app_timer_cancel(s_ignore_single_click_timer);
        s_ignore_single_click_timer = NULL;
    }

    ui_activity_picker_destroy();
#if defined(PBL_COLOR)
    ui_color_picker_destroy();
    ui_intermediate_menu_destroy();
#endif

    if (s_time_hour_layer) text_layer_destroy(s_time_hour_layer);
    if (s_time_colon1_layer) text_layer_destroy(s_time_colon1_layer);
    if (s_time_min_layer) text_layer_destroy(s_time_min_layer);
    if (s_time_colon2_layer) text_layer_destroy(s_time_colon2_layer);
    if (s_time_sec_layer) text_layer_destroy(s_time_sec_layer);
    if (s_clock_layer) text_layer_destroy(s_clock_layer);
    if (s_dist_layer) text_layer_destroy(s_dist_layer);
    if (s_step_layer) text_layer_destroy(s_step_layer);
    if (s_hr_layer) text_layer_destroy(s_hr_layer);
    
    if (s_mid_bg_layer) layer_destroy(s_mid_bg_layer);
    if (s_graph_layer) layer_destroy(s_graph_layer);
    if (s_action_bar) action_bar_layer_destroy(s_action_bar);
    
    if (s_icons_loaded) {
        if (s_icon_play) gbitmap_destroy(s_icon_play);
        if (s_icon_pause) gbitmap_destroy(s_icon_pause);
        if (s_icon_stop) gbitmap_destroy(s_icon_stop);
        if (s_icon_check) gbitmap_destroy(s_icon_check);
        if (s_icon_trash) gbitmap_destroy(s_icon_trash);
        if (s_icon_up) gbitmap_destroy(s_icon_up);
        if (s_icon_down) gbitmap_destroy(s_icon_down);
        if (s_icon_graph) gbitmap_destroy(s_icon_graph);
        if (s_icon_save) gbitmap_destroy(s_icon_save);
        if (s_icon_setting) gbitmap_destroy(s_icon_setting);
    }
}

static void init(void) {
    graph_data_init();
    comm_service_init(update_ui_state, on_graph_dirty_request);
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
    touch_service_subscribe(touch_event_handler, NULL);
#endif
    
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);
}

static void deinit(void) {
#if defined(PBL_HEALTH)
    if (s_has_hr_sensor) {
        health_service_set_heart_rate_sample_period(0);
    }
#endif
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
    touch_service_unsubscribe();
#endif
    window_destroy(s_main_window);
    tick_timer_service_unsubscribe();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
