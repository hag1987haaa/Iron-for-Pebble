#include <pebble.h>

/* ==========================================================
   定数・マクロ・フォント定義
   ========================================================== */

#define KEY_STEPS 10010
#define KEY_HR 10007
#define MESSAGE_KEY_CMD 10000
#define MESSAGE_KEY_TIME 10001
#define MESSAGE_KEY_DISTANCE 10002
#define MESSAGE_KEY_PACE 10003
#define MESSAGE_KEY_STATE 10004
#define MESSAGE_KEY_MEDIA_CMD 10008  
#define MESSAGE_KEY_GRAPH_DATA 10009
#define MESSAGE_KEY_MID_DATA 10013
#define MESSAGE_KEY_ACTIVITY_TYPE 10012

#define PK_START_STEPS 51
#define PK_GRAPH_COUNT 53
#define PK_GRAPH_DATA 54
#define PK_GRAPH_ID 55
#define PK_GRAPH_SCALE 56
#define PK_PERSONAL_COLOR 57 
#define PK_ACTIVITY_TYPE 59

typedef enum {
  ACTIVITY_RUNNING = 0,
  ACTIVITY_WALKING = 1,
  ACTIVITY_CYCLING = 2,
  ACTIVITY_HIKING  = 3,
  ACTIVITY_KAYAKING = 4,
  ACTIVITY_ROWING  = 5,
  ACTIVITY_OTHER   = 6,
  ACTIVITY_COUNT   = 7
} ActivityType;

static ActivityType s_current_activity = ACTIVITY_RUNNING;

static const char* ACTIVITY_NAMES[] = {
  "RUNNING", "WALKING", "CYCLING", "HIKING", 
  "KAYAKING", "ROWING", "OTHER"
};

static Layer *s_activity_picker_layer = NULL;
static bool s_is_activity_picking = false;
static int s_preview_activity_idx = 0;

#define MAX_MID_PAGES 15
typedef struct {
    char name[16];
    char value[16];
    char unit[16];
    int icon_id;
} MidPageData;

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
#define SWIPE_MIN_DIST_PX 30      
#define SWIPE_MAX_TIME_MS 800
#define DOUBLE_TAP_MAX_DELAY_MS 500
#define TAP_MAX_DIST_PX 15
#endif

/* ==========================================================
   グローバル変数
   ========================================================== */
static Window *s_main_window;
static TextLayer *s_time_hour_layer, *s_time_colon1_layer, *s_time_min_layer, *s_time_colon2_layer, *s_time_sec_layer; 
static TextLayer *s_clock_layer, *s_dist_layer, *s_hr_layer, *s_step_layer;
static TextLayer *s_msg_layer = NULL;
static Layer *s_msg_container_layer = NULL; 
static Layer *s_graph_layer, *s_mid_bg_layer;

#if defined(PBL_COLOR)
static Layer *s_color_picker_layer = NULL;
static bool s_is_color_picking = false;
static int s_selected_color_idx = 0; 
static bool s_preview_is_running = false; 
static uint8_t s_personal_color_argb = GColorIslamicGreenARGB8; 

static Layer *s_intermediate_layer = NULL;
static bool s_is_intermediate_menu = false;
static int s_intermediate_idx = 0; 

#endif

static GFont s_font_huge_time;
static GFont s_font_long_time;
static GFont s_font_colon;
static GFont s_font_mid_data;

static GColor s_current_main_bg, s_current_main_fg;
static PropertyAnimation *s_marquee_anim = NULL;
static AppTimer *s_marquee_timer = NULL; 
static ActionBarLayer *s_action_bar = NULL;

static GBitmap *s_icon_play, *s_icon_pause, *s_icon_stop, *s_icon_check, *s_icon_trash, *s_icon_up, *s_icon_down, *s_icon_graph, *s_icon_save, *s_icon_setting;
static bool s_icons_loaded = false, s_current_icon_color_is_black = false;

static char s_time_hour_buf[8] = "0", s_time_min_buf[8] = "00", s_time_sec_buf[8] = "00", s_clock_buf[16] = "--:--", s_dist_buf[16] = "--", s_hr_buf[16] = "--", s_step_buf[16] = "--";

static uint8_t s_app_state = 0; 
static bool s_is_paused = true;
static bool s_is_small_screen = false;
static bool s_is_round_screen = false;
static bool s_is_long_workout = false;
static bool s_has_hr_sensor = false; 
static bool s_is_custom_marquee = false;

static int s_start_steps = 0, s_pause_start_steps = 0, s_current_hr = 0;

#define MAX_GRAPH_DATA 45 
static int s_graph_data[MAX_GRAPH_DATA]; 
static int s_graph_count = 0, s_graph_id = 0, s_graph_scale = 1; 

static MidPageData s_mid_pages[MAX_MID_PAGES];
static int s_mid_page_count = 0;
static int s_current_mid_mode = 0; // 0 ~ s_mid_page_count-1

static char s_graph_y_label[16] = "";
static char s_graph_x_label[16] = "";
static char s_graph_max_label[16] = "";
static char s_graph_min_label[16] = "";

static GRect s_rect_hour_5, s_rect_col1_5, s_rect_min_5, s_rect_col2_5, s_rect_sec_5, s_rect_min_3, s_rect_col2_3, s_rect_sec_3;

/* ==========================================================
   プロトタイプ宣言（コンパイルエラー防止）
   ========================================================== */
static void trigger_marquee();
static void trigger_custom_marquee(const char *msg);
static void stop_marquee();
static void update_ui_state();
static void click_config_provider(void *context);
static void destroy_marquee_layers();

#if defined(PBL_COLOR)
static void color_picker_update_proc(Layer *layer, GContext *ctx);
static void create_color_picker_layer();
static void destroy_color_picker_layer();

static void intermediate_update_proc(Layer *layer, GContext *ctx);
static void create_intermediate_layer();
static void destroy_intermediate_layer();

#endif

static void activity_picker_update_proc(Layer *layer, GContext *ctx);
static void create_activity_picker_layer();
static void destroy_activity_picker_layer();

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
static void touch_event_handler(const TouchEvent *event, void *context);
static void send_media_cmd(int cmd);
#endif
static uint64_t get_current_time_ms();

/* ==========================================================
   ユーティリティ関数
   ========================================================== */
static int32_t get_int(Tuple *t) {
    if (!t) return 0;
    if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
    if (t->length == 1) return (int32_t)t->value->uint8;
    if (t->length == 2) return (int32_t)t->value->uint16;
    if (t->length == 4) return (int32_t)t->value->uint32;
    return 0;
}

static void send_cmd(int val) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_CMD, val);
        app_message_outbox_send();
    }
}

static void send_activity_type_to_phone(ActivityType type) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_ACTIVITY_TYPE, (int32_t)type);
        app_message_outbox_send();
    }
}

static uint64_t get_current_time_ms() {
    time_t s;
    uint16_t ms;
    time_ms(&s, &ms);
    return ((uint64_t)s * 1000) + ms;
}

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
static void send_media_cmd(int cmd) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_MEDIA_CMD, cmd);
        app_message_outbox_send();
    }
}

#endif

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

static void save_graphs_to_persist() {
    persist_write_int(PK_GRAPH_COUNT, s_graph_count);
    persist_write_data(PK_GRAPH_DATA, s_graph_data, sizeof(s_graph_data));
    persist_write_int(PK_GRAPH_ID, s_graph_id);
    persist_write_int(PK_GRAPH_SCALE, s_graph_scale);
    persist_write_int(PK_START_STEPS, s_start_steps);
}

static void load_persist_data() {
    if (persist_exists(PK_START_STEPS)) s_start_steps = persist_read_int(PK_START_STEPS);
    if (persist_exists(PK_GRAPH_COUNT)) s_graph_count = persist_read_int(PK_GRAPH_COUNT);
    if (persist_exists(PK_GRAPH_DATA)) persist_read_data(PK_GRAPH_DATA, s_graph_data, sizeof(s_graph_data));
    if (persist_exists(PK_GRAPH_ID)) s_graph_id = persist_read_int(PK_GRAPH_ID);
    if (persist_exists(PK_GRAPH_SCALE)) s_graph_scale = persist_read_int(PK_GRAPH_SCALE);
    if (persist_exists(PK_ACTIVITY_TYPE)) s_current_activity = (ActivityType)persist_read_int(PK_ACTIVITY_TYPE);
#if defined(PBL_COLOR)
    if (persist_exists(PK_PERSONAL_COLOR)) {
        s_selected_color_idx = persist_read_int(PK_PERSONAL_COLOR);
        s_personal_color_argb = (GColor){.argb = s_selected_color_idx + 0b11000000}.argb;
    }
#endif
}

static void clear_graph_data() {
    s_graph_count = 0;
    s_graph_id = 0;
    s_graph_scale = 1;
    memset(s_graph_data, 0, sizeof(s_graph_data));
#if defined(PBL_HEALTH)
    s_start_steps = (int)health_service_sum_today(HealthMetricStepCount);
#else
    s_start_steps = 0;
#endif
    save_graphs_to_persist();
    if (s_graph_layer) layer_mark_dirty(s_graph_layer);
}

// 安全に文字列を区切り文字まで抽出するヘルパー関数
static void extract_token(const char **ptr, char *out, int max_len) {
    int i = 0;
    while (**ptr && **ptr != ',' && **ptr != '|') {
        if (i < max_len - 1) out[i++] = **ptr;
        (*ptr)++;
    }
    out[i] = '\0';
    if (**ptr == ',' || **ptr == '|') (*ptr)++;
}

static void parse_hybrid_graph_data(const char *input) {
    if (!input || input[0] == '\0') {
        s_graph_count = 0;
        return;
    }
    memset(s_graph_data, 0, sizeof(s_graph_data));
    const char *p = input;
    
    char type_str[8];
    extract_token(&p, type_str, sizeof(type_str));
    s_graph_id = atoi(type_str); 
    
    extract_token(&p, s_graph_y_label, sizeof(s_graph_y_label));
    extract_token(&p, s_graph_x_label, sizeof(s_graph_x_label));
    extract_token(&p, s_graph_max_label, sizeof(s_graph_max_label));
    extract_token(&p, s_graph_min_label, sizeof(s_graph_min_label));
    
    int idx = 0;
    while (*p && idx < MAX_GRAPH_DATA) {
        char val_str[16];
        extract_token(&p, val_str, sizeof(val_str));
        if (val_str[0] != '\0') {
            s_graph_data[idx++] = atoi(val_str);
        }
    }
    s_graph_count = idx;
}

static void parse_mid_data(const char *input) {
    s_mid_page_count = 0;
    if (!input || input[0] == '\0') return;
    const char *p = input;
    while (*p && s_mid_page_count < MAX_MID_PAGES) {
        MidPageData *page = &s_mid_pages[s_mid_page_count];
        extract_token(&p, page->name, sizeof(page->name));
        extract_token(&p, page->value, sizeof(page->value));
        extract_token(&p, page->unit, sizeof(page->unit));
        char icon_str[8];
        extract_token(&p, icon_str, sizeof(icon_str));
        page->icon_id = atoi(icon_str);
        s_mid_page_count++;
    }
    if (s_mid_page_count == 0 || s_current_mid_mode >= s_mid_page_count) {
        s_current_mid_mode = 0;
    }
}

static AppTimer *s_ignore_single_click_timer = NULL;
static bool s_ignore_single_click = false;
static uint64_t s_long_click_start_time = 0;

static void reset_ignore_single_click_callback(void *context) {
    s_ignore_single_click = false;
    s_ignore_single_click_timer = NULL;
}

static void trigger_ignore_single_click() {
    s_ignore_single_click = true;
    if (s_ignore_single_click_timer) app_timer_cancel(s_ignore_single_click_timer);
    s_ignore_single_click_timer = app_timer_register(600, reset_ignore_single_click_callback, NULL);
}

/* ==========================================================
   イベントハンドラ群 (Touch, Click)
   ========================================================== */
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
static void touch_event_handler(const TouchEvent *event, void *context) {
    uint64_t current_time = get_current_time_ms();
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
                    if (dx > 0) send_media_cmd(3); // PREV
                    else send_media_cmd(2);        // NEXT
                } else if (abs_dy > abs_dx * 2) {
                    if (dy > 0) send_media_cmd(5); // VOL DOWN
                    else send_media_cmd(4);        // VOL UP
                }
                vibes_short_pulse();
                s_last_tap_time = 0; 
            } 
            else if (abs_dx <= TAP_MAX_DIST_PX && abs_dy <= TAP_MAX_DIST_PX) {
                if (s_last_tap_time != 0 && (current_time - s_last_tap_time) < DOUBLE_TAP_MAX_DELAY_MS) {
                    send_media_cmd(1); // PLAY/PAUSE
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
    if (s_is_color_picking && s_action_bar) {
        s_selected_color_idx = (s_selected_color_idx - 1 + 64) % 64;
        s_personal_color_argb = (GColor){.argb = s_selected_color_idx + 0b11000000}.argb;
        update_ui_state();
        return;
    }
    if (s_is_intermediate_menu && s_action_bar) {
        s_intermediate_idx = (s_intermediate_idx - 1 + 2) % 2;
        if (s_intermediate_layer) layer_mark_dirty(s_intermediate_layer);
        return;
    }
#endif
    if (s_is_activity_picking && s_action_bar) {
        s_preview_activity_idx = (s_preview_activity_idx - 1 + ACTIVITY_COUNT) % ACTIVITY_COUNT;
        if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
        return;
    }
    if (s_ignore_single_click) return;

    if (s_app_state <= 4) {
        send_cmd(1);
        vibes_short_pulse();
    } else if (s_app_state == 5) {
        send_cmd(7);
        vibes_short_pulse();
    }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
#if defined(PBL_COLOR)
    if (s_is_color_picking && s_action_bar) {
        persist_write_int(PK_PERSONAL_COLOR, s_selected_color_idx);
        s_is_color_picking = false;
        destroy_color_picker_layer();
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
    if (s_is_activity_picking && s_action_bar) {
        s_current_activity = (ActivityType)s_preview_activity_idx;
        persist_write_int(PK_ACTIVITY_TYPE, s_current_activity);
        send_activity_type_to_phone(s_current_activity);
        s_is_activity_picking = false;
        destroy_activity_picker_layer();
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
    if (s_is_intermediate_menu && s_action_bar) {
        s_is_intermediate_menu = false;
        destroy_intermediate_layer();
        if (s_intermediate_idx == 0) {
            s_is_activity_picking = true;
            s_preview_activity_idx = s_current_activity;
            create_activity_picker_layer();
        } else {
            s_is_color_picking = true;
            s_preview_is_running = false;
            create_color_picker_layer();
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
        if (s_mid_page_count > 0) {
            s_current_mid_mode++;
            if (s_current_mid_mode >= s_mid_page_count) s_current_mid_mode = 0;
            update_ui_state();
            vibes_short_pulse();
        }
        return;
    }

    if (s_app_state <= 2) {
#if defined(PBL_COLOR)
        // For color devices, show the intermediate menu
        s_is_intermediate_menu = true;
        s_intermediate_idx = 0;
        create_intermediate_layer();
#else
        // For monochrome devices, go directly to the activity picker
        s_is_activity_picking = true;
        s_preview_activity_idx = s_current_activity;
        create_activity_picker_layer();
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
        send_cmd(2);
        vibes_short_pulse();
    } else if (s_app_state == 6) {
        send_cmd(9);
        vibes_short_pulse();
    }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
#if defined(PBL_COLOR)
    if (s_is_color_picking && s_action_bar) {
        s_selected_color_idx = (s_selected_color_idx + 1) % 64;
        s_personal_color_argb = (GColor){.argb = s_selected_color_idx + 0b11000000}.argb;
        update_ui_state();
        return;
    }
    if (s_is_intermediate_menu && s_action_bar) {
        s_intermediate_idx = (s_intermediate_idx + 1) % 2;
        if (s_intermediate_layer) layer_mark_dirty(s_intermediate_layer);
        return;
    }
#endif
    if (s_is_activity_picking && s_action_bar) {
        s_preview_activity_idx = (s_preview_activity_idx + 1) % ACTIVITY_COUNT;
        if (s_activity_picker_layer) layer_mark_dirty(s_activity_picker_layer);
        return;
    }
    if (s_ignore_single_click) return;

    if (s_app_state != 5) {
        send_cmd(6);
        vibes_short_pulse();
    } else if (s_app_state == 5) {
        send_cmd(8);
        vibes_short_pulse();
    }
}

static void generic_long_click_down_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = s_is_activity_picking;
#if defined(PBL_COLOR)
    ignore = ignore || s_is_color_picking || s_is_intermediate_menu;
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    s_long_click_start_time = get_current_time_ms();
    vibes_short_pulse();
}

static void up_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = s_is_activity_picking;
#if defined(PBL_COLOR)
    ignore = ignore || s_is_color_picking || s_is_intermediate_menu;
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    
    // 800ms + 1200ms = 2000ms (2秒) 以上押されていた場合はキャンセル
    if (get_current_time_ms() - s_long_click_start_time >= 1200) return;

    send_cmd(50);
    trigger_custom_marquee("UP LONG SEND");
    vibes_short_pulse();
}

static void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = s_is_activity_picking;
#if defined(PBL_COLOR)
    ignore = ignore || s_is_color_picking || s_is_intermediate_menu;
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    
    // 2秒以上押されていた場合はキャンセル
    if (get_current_time_ms() - s_long_click_start_time >= 1200) return;

    send_cmd(51);
    trigger_custom_marquee("SELECT LONG SEND");
    vibes_short_pulse();
}

static void down_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
    bool ignore = s_is_activity_picking;
#if defined(PBL_COLOR)
    ignore = ignore || s_is_color_picking || s_is_intermediate_menu;
#endif
    if (ignore) return;
    trigger_ignore_single_click();
    
    // 2秒以上押されていた場合はキャンセル
    if (get_current_time_ms() - s_long_click_start_time >= 1200) return;

    send_cmd(52);
    trigger_custom_marquee("DOWN LONG SEND");
    vibes_short_pulse();
}

static void click_config_provider(void *context) {
    bool custom_clicks = s_is_activity_picking;
#if defined(PBL_COLOR)
    custom_clicks = custom_clicks || s_is_color_picking || s_is_intermediate_menu;
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
   カラーピッカー
   ========================================================== */
#if defined(PBL_COLOR)
static void color_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_color_picking) return;
    
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_main_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 18;
    graphics_context_set_text_color(ctx, s_current_main_fg);
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

static void create_color_picker_layer() {
    if (s_color_picker_layer != NULL) return;
    Layer *wl = window_get_root_layer(s_main_window);
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
    if (s_action_bar) {
        layer_insert_below_sibling(s_color_picker_layer, action_bar_layer_get_layer(s_action_bar));
    } else {
        layer_add_child(wl, s_color_picker_layer);
    }
}

static void destroy_color_picker_layer() {
    if (s_color_picker_layer) {
        layer_destroy(s_color_picker_layer);
        s_color_picker_layer = NULL;
    }
}

static void intermediate_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_intermediate_menu) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_main_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 18;
    graphics_context_set_text_color(ctx, s_current_main_fg);
    graphics_draw_text(ctx, "SETTINGS", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);
    
    int item_h = 28;
    int oy = text_h + 10;
    
    for (int i = 0; i < 2; i++) {
        if (s_intermediate_idx == i) {
            graphics_context_set_fill_color(ctx, s_current_main_fg);
            graphics_fill_rect(ctx, GRect(5, oy, b.size.w - 10, item_h + 2), 4, GCornersAll);
            graphics_context_set_text_color(ctx, s_current_main_bg);
        } else {
            graphics_context_set_text_color(ctx, s_current_main_fg);
        }
        const char *label = (i == 0) ? "Activity Type" : "Custom Color";
        graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0, oy + 2, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
        oy += item_h + 10;
    }
}

static void create_intermediate_layer() {
    if (s_intermediate_layer != NULL) return;
    Layer *wl = window_get_root_layer(s_main_window);
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
    if (s_action_bar) layer_insert_below_sibling(s_intermediate_layer, action_bar_layer_get_layer(s_action_bar));
    else layer_add_child(wl, s_intermediate_layer);
}

static void destroy_intermediate_layer() {
    if (s_intermediate_layer) {
        layer_destroy(s_intermediate_layer);
        s_intermediate_layer = NULL;
    }
}

#endif

static void activity_picker_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_activity_picking) return;
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_main_bg);
    graphics_fill_rect(ctx, b, 0, GCornerNone);
    
    int text_h = 18;
    graphics_context_set_text_color(ctx, s_current_main_fg);
    graphics_draw_text(ctx, "ACTIVITY TYPE", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, b.size.w, text_h), 0, GTextAlignmentCenter, NULL);
    
    int item_h = 24;
    int oy = text_h + 6;
    int prev_idx = (s_preview_activity_idx - 1 + ACTIVITY_COUNT) % ACTIVITY_COUNT;
    int next_idx = (s_preview_activity_idx + 1) % ACTIVITY_COUNT;
    
    graphics_context_set_text_color(ctx, s_current_main_fg);
    graphics_draw_text(ctx, ACTIVITY_NAMES[prev_idx], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, oy, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
    oy += item_h + 4;
    
    graphics_context_set_fill_color(ctx, s_current_main_fg);
    graphics_fill_rect(ctx, GRect(5, oy, b.size.w - 10, item_h + 4), 4, GCornersAll);
    graphics_context_set_text_color(ctx, s_current_main_bg);
    graphics_draw_text(ctx, ACTIVITY_NAMES[s_preview_activity_idx], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0, oy + 2, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
    oy += item_h + 8;
    
    graphics_context_set_text_color(ctx, s_current_main_fg);
    graphics_draw_text(ctx, ACTIVITY_NAMES[next_idx], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, oy, b.size.w, item_h), 0, GTextAlignmentCenter, NULL);
}

static void create_activity_picker_layer() {
    if (s_activity_picker_layer != NULL) return;
    Layer *wl = window_get_root_layer(s_main_window);
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
    if (s_action_bar) layer_insert_below_sibling(s_activity_picker_layer, action_bar_layer_get_layer(s_action_bar));
    else layer_add_child(wl, s_activity_picker_layer);
}

static void destroy_activity_picker_layer() {
    if (s_activity_picker_layer) {
        layer_destroy(s_activity_picker_layer);
        s_activity_picker_layer = NULL;
    }
}

/* ==========================================================
   アニメーション・Marquee (流れ文字)
   ========================================================== */
static void msg_container_update_proc(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, s_current_main_bg);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void marquee_timer_callback(void *context) {
    s_marquee_timer = NULL;
    trigger_marquee(); 
}

static void stop_marquee() {
    if (s_marquee_anim) {
        animation_unschedule(property_animation_get_animation(s_marquee_anim));
    }
    if (s_marquee_timer) {
        app_timer_cancel(s_marquee_timer);
        s_marquee_timer = NULL;
    }
}

static void destroy_marquee_layers() {
    stop_marquee();
    if (s_msg_layer) {
        text_layer_destroy(s_msg_layer);
        s_msg_layer = NULL;
    }
    if (s_msg_container_layer) {
        layer_destroy(s_msg_container_layer);
        s_msg_container_layer = NULL;
    }
    s_is_custom_marquee = false;
    if (s_graph_layer) layer_mark_dirty(s_graph_layer);
}

static void create_marquee_layers() {
    if (s_msg_container_layer != NULL) return;
    
    Layer *wl = window_get_root_layer(s_main_window);
    GRect b = layer_get_bounds(wl);
    
#if defined(PBL_ROUND)
    int w = b.size.w;
    int h = b.size.h;
    int mid_h = (w >= 260) ? 65 : 45;
    int upper_h = (h - mid_h) / 2;
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

    if (s_graph_layer) layer_mark_dirty(s_graph_layer);
}

static void anim_stopped_handler(Animation *animation, bool finished, void *context) {
    if (s_marquee_anim && property_animation_get_animation(s_marquee_anim) == animation) {
        s_marquee_anim = NULL;
    }
    
    if (finished && (s_app_state == 5 || s_app_state == 6)) {
        destroy_marquee_layers();
        if (s_marquee_timer) app_timer_cancel(s_marquee_timer);
        s_marquee_timer = app_timer_register(5000, marquee_timer_callback, NULL);
    } else if (finished && (s_app_state < 3)) {
        trigger_marquee();
    } else if (finished) {
        destroy_marquee_layers();
    }
}

static void custom_msg_timer_callback(void *context) {
    s_marquee_timer = NULL;
    s_is_custom_marquee = false;
    if (s_app_state == 5 || s_app_state == 6) {
        destroy_marquee_layers();
        s_marquee_timer = app_timer_register(5000, marquee_timer_callback, NULL);
    } else if (s_app_state < 3) {
        trigger_marquee();
    } else {
        destroy_marquee_layers();
    }
}

static void trigger_custom_marquee(const char *msg) {
    s_is_custom_marquee = true;
    create_marquee_layers();
    stop_marquee();
    
    static char custom_msg_buf[64];
    snprintf(custom_msg_buf, sizeof(custom_msg_buf), "%s", msg);
    
    text_layer_set_text(s_msg_layer, custom_msg_buf);
    text_layer_set_text_color(s_msg_layer, s_current_main_fg);
    text_layer_set_text_alignment(s_msg_layer, GTextAlignmentCenter);
    
    Layer *wl = window_get_root_layer(s_main_window);
#if defined(PBL_ROUND)
    int w = layer_get_bounds(wl).size.w;
#else
    int w = layer_get_bounds(wl).size.w - ACTION_BAR_WIDTH;
#endif
    
    layer_set_frame(text_layer_get_layer(s_msg_layer), GRect(0, -2, w, 24));
    
    s_marquee_timer = app_timer_register(3000, custom_msg_timer_callback, NULL);
}

static void trigger_marquee() {
    if (s_is_custom_marquee) return;

    if (s_app_state == 3 || s_app_state == 4) {
        destroy_marquee_layers();
        return;
    }

    create_marquee_layers();
    stop_marquee();
    
    static char msg_buf[64];
    if (s_app_state == 0) {
        snprintf(msg_buf, sizeof(msg_buf), "PRESS [UP] TO START OR SET UP ON PHONE ...");
    } else if (s_app_state == 1) {
        snprintf(msg_buf, sizeof(msg_buf), "SEARCHING GPS ...");
    } else if (s_app_state == 2) {
        snprintf(msg_buf, sizeof(msg_buf), "READY TO START !");
    } else if (s_app_state == 5) {
        snprintf(msg_buf, sizeof(msg_buf), "FINISH? [UP] SAVE [DOWN] DISCARD");
    } else if (s_app_state == 6) {
        snprintf(msg_buf, sizeof(msg_buf), "SAVED ! PRESS SELECT TO RESET");
    } else {
        return;
    }
    
    text_layer_set_text(s_msg_layer, msg_buf);
    text_layer_set_text_color(s_msg_layer, s_current_main_fg);
    text_layer_set_text_alignment(s_msg_layer, GTextAlignmentLeft);
    
    Layer *wl = window_get_root_layer(s_main_window);
#if defined(PBL_ROUND)
    int w = layer_get_bounds(wl).size.w;
#else
    int w = layer_get_bounds(wl).size.w - ACTION_BAR_WIDTH;
#endif
    int text_w = 450;
    
    GRect start = GRect(w, -2, text_w, 24);
    GRect finish = GRect(-text_w, -2, text_w, 24);
    
    s_marquee_anim = property_animation_create_layer_frame(text_layer_get_layer(s_msg_layer), &start, &finish);
    if (s_marquee_anim) {
        Animation *anim = property_animation_get_animation(s_marquee_anim);
        animation_set_duration(anim, 4500);
        animation_set_curve(anim, AnimationCurveLinear);
        animation_set_handlers(anim, (AnimationHandlers) { .stopped = anim_stopped_handler }, NULL);
        animation_schedule(anim);
    }
}

/* ==========================================================
   描画コールバック関数 (Graph, Mid_BG)
   ========================================================== */
static void graph_update_proc(Layer *layer, GContext *ctx) {
    if (s_app_state < 3 && s_app_state != 6) return;
    
    GRect b = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, s_current_main_fg);
    graphics_context_set_stroke_color(ctx, s_current_main_fg);
    graphics_context_set_text_color(ctx, s_current_main_fg);
    
    int pl = 4;
    int tm = 16;
    int ah = b.size.h - tm;
    int dw = b.size.w - pl;
    
    int bw = dw / (s_graph_count > 0 ? s_graph_count : 1);
    if (bw < 2) bw = 2;
    
    bool show_labels = (s_msg_container_layer == NULL);
    
    if (show_labels) {
        // スマホから送られてきたラベルをそのまま描画
        graphics_draw_text(ctx, s_graph_y_label, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(pl, 4, b.size.w, 16), 0, GTextAlignmentLeft, NULL);
        graphics_draw_text(ctx, s_graph_x_label, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, 4, b.size.w - 4, 16), 0, GTextAlignmentRight, NULL);
    }
    
    if (s_graph_count <= 0) return;
    
    int mah = (ah * 70) / 100;
    int prev_x = -1, prev_y = -1;
    
    if (s_graph_id == 0) { 
        int p_min = 999999, p_max = -1;
        for (int i = 0; i < s_graph_count; i++) {
            if (s_graph_data[i] > 0) { 
                if (s_graph_data[i] > p_max) p_max = s_graph_data[i];
                if (s_graph_data[i] < p_min) p_min = s_graph_data[i];
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
        
        for (int i = 0; i < s_graph_count; i++) {
            int x = pl + i * bw + (bw / 2);
            int y;
            if (s_graph_data[i] <= 0) {
                y = b.size.h; 
            } else {
                int val = s_graph_data[i];
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
    } else if (s_graph_id == 1) {
        int p_max = 1;
        for (int i = 0; i < s_graph_count; i++) {
            if (s_graph_data[i] > p_max) p_max = s_graph_data[i];
        }
        for (int i = 0; i < s_graph_count; i++) {
            int bh = (s_graph_data[i] * mah) / p_max;
            if (bh < 0) bh = 0;
            int bar_w = bw - 1;
            if (bar_w < 1) bar_w = 1; 
            graphics_fill_rect(ctx, GRect(pl + i * bw, b.size.h - bh, bar_w, bh), 0, GCornerNone);
        }
    } else if (s_graph_id == 2) {
        int p_min = 999999, p_max = -1;
        for (int i = 0; i < s_graph_count; i++) {
            if (s_graph_data[i] > 0) {
                if (s_graph_data[i] > p_max) p_max = s_graph_data[i];
                if (s_graph_data[i] < p_min) p_min = s_graph_data[i];
            }
        }
        if (p_min == 999999) { p_min = 60; p_max = 120; }
        if (p_max == p_min) p_max = p_min + 10;

        int plot_max = p_max + 10;
        int plot_min = p_min - 20;
        if (plot_min < 0) plot_min = 0;
        int plot_range = plot_max - plot_min;

        for (int i = 0; i < s_graph_count; i++) {
            int val = s_graph_data[i];
            if (val <= 0) continue; 

            if (val > plot_max) val = plot_max;
            if (val < plot_min) val = plot_min;
            
            int bh = ((val - plot_min) * mah) / plot_range;
            if (bh < 2) bh = 2; 
            
            int bar_w = bw - 1;
            if (bar_w < 1) bar_w = 1;
            graphics_fill_rect(ctx, GRect(pl + i * bw, b.size.h - bh, bar_w, bh), 0, GCornerNone);
        }
    } else if (s_graph_id == 3) {
        int p_min = 999999, p_max = -999999;
        for (int i = 0; i < s_graph_count; i++) {
            if (s_graph_data[i] > p_max) p_max = s_graph_data[i];
            if (s_graph_data[i] < p_min) p_min = s_graph_data[i];
        }
        if (p_min == 999999) { p_min = 0; p_max = 100; }
        if (p_max == p_min) { p_max = p_min + 10; p_min -= 10; }
        
        int margin = (p_max - p_min) / 4;
        if (margin < 5) margin = 5;
        int plot_min = p_min - margin;
        int plot_max = p_max + margin;
        int plot_range = plot_max - plot_min;

        for (int i = 0; i < s_graph_count; i++) {
            int x = pl + i * bw + (bw / 2);
            int val = s_graph_data[i];
            
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
    } else if (s_graph_id == 4) {
        int p_min = 999999, p_max = -1;
        for (int i = 0; i < s_graph_count; i++) {
            if (s_graph_data[i] > 0) {
                if (s_graph_data[i] > p_max) p_max = s_graph_data[i];
                if (s_graph_data[i] < p_min) p_min = s_graph_data[i];
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

        for (int i = 0; i < s_graph_count; i++) {
            int x = pl + i * bw + (bw / 2);
            int y;
            if (s_graph_data[i] <= 0) {
                y = b.size.h;
            } else {
                int val = s_graph_data[i];
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
            char max_str[8], min_str[8];
            snprintf(max_str, 8, "%d", p_max);
            snprintf(min_str, 8, "%d", p_min);
            
            int lbl_w = 28;
            int lbl_x = b.size.w - lbl_w;
            int lbl_h = 16;
            graphics_draw_text(ctx, max_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - mah - 2, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
            graphics_draw_text(ctx, min_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - 16, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
        }
    } else if (s_graph_id == 5) {
        // Calories: 棒グラフ表現と最大値(kcal)の表示
        int p_max = 1;
        for (int i = 0; i < s_graph_count; i++) {
            if (s_graph_data[i] > p_max) p_max = s_graph_data[i];
        }
        for (int i = 0; i < s_graph_count; i++) {
            int bh = (s_graph_data[i] * mah) / p_max;
            if (bh < 0) bh = 0;
            int bar_w = bw - 1;
            if (bar_w < 1) bar_w = 1; 
            graphics_fill_rect(ctx, GRect(pl + i * bw, b.size.h - bh, bar_w, bh), 0, GCornerNone);
        }
        if (show_labels) {
            int lbl_w = 48;
            int lbl_x = b.size.w - lbl_w;
            int lbl_h = 16;
            graphics_draw_text(ctx, s_graph_max_label, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(lbl_x, b.size.h - mah - 2, lbl_w, lbl_h), 0, GTextAlignmentRight, NULL);
        }
    }
}

static bool is_detail_mode() {
    if (s_app_state != 3) return true;
    if (s_mid_page_count == 0) return true;
    if (s_current_mid_mode < s_mid_page_count) {
        MidPageData *page = &s_mid_pages[s_current_mid_mode];
        if (page->name[0] == '\0' && strcmp(page->value, "DETAIL") == 0) {
            return true;
        }
    }
    return false;
}

static void mid_bg_update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    graphics_context_set_stroke_color(ctx, s_current_main_fg);
    graphics_context_set_fill_color(ctx, s_current_main_fg);
    graphics_context_set_text_color(ctx, s_current_main_fg);

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
    bool is_active = (s_app_state == 3);

    if (is_active) {
        // 中段エリアを塗りつぶし（背景色と文字色の反転）
        graphics_context_set_fill_color(ctx, s_current_main_fg);
        graphics_fill_rect(ctx, GRect(0, upper_h, fill_w, mid_h), 0, GCornerNone);
        graphics_context_set_text_color(ctx, s_current_main_bg);
        graphics_context_set_stroke_color(ctx, s_current_main_bg);
    } else {
        graphics_context_set_text_color(ctx, s_current_main_fg);
        graphics_context_set_stroke_color(ctx, s_current_main_fg);
    }

    if (!is_detail_mode()) {

        if (s_current_mid_mode < s_mid_page_count) {
            MidPageData *page = &s_mid_pages[s_current_mid_mode];
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

            graphics_draw_text(ctx, page->value, s_font_long_time, GRect(5, val_y, active_w - 10, 48), 0, GTextAlignmentCenter, NULL);
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
    
    graphics_context_set_fill_color(ctx, is_active ? s_current_main_bg : s_current_main_fg);
    graphics_fill_circle(ctx, GPoint(lx, r1y), 3);
    graphics_draw_line(ctx, GPoint(lx - 2, r1y + 2), GPoint(lx, r1y + 6));
    graphics_draw_line(ctx, GPoint(lx + 2, r1y + 2), GPoint(lx, r1y + 6));
    
    graphics_fill_rect(ctx, GRect(rx - 1, r1y, 3, 5), 1, GCornerNone);
    graphics_fill_circle(ctx, GPoint(rx + 2, r1y - 2), 1);
    graphics_fill_circle(ctx, GPoint(rx, r1y - 3), 1);

    if (s_has_hr_sensor) {
        graphics_fill_circle(ctx, GPoint(lx - 2, r2y), 2);
        graphics_fill_circle(ctx, GPoint(lx + 2, r2y), 2);
        graphics_draw_line(ctx, GPoint(lx - 4, r2y + 1), GPoint(lx, r2y + 5));
        graphics_draw_line(ctx, GPoint(lx + 4, r2y + 1), GPoint(lx, r2y + 5));
    }

    graphics_draw_circle(ctx, GPoint(rx, r2y), 4);
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx, r2y - 2));
    graphics_draw_line(ctx, GPoint(rx, r2y), GPoint(rx + 2, r2y));
}

static void update_ui_state() {
    GColor pc = GColorWhite;
#if defined(PBL_COLOR)
    pc = (GColor){.argb = s_personal_color_argb};
#endif

    bool is_active = (s_app_state == 3);

#if defined(PBL_COLOR)
    bool pc_is_bright = gcolor_equal(gcolor_legible_over(pc), GColorBlack);

    if (s_is_color_picking && s_action_bar) {
        s_current_main_bg = s_preview_is_running ? pc : gcolor_legible_over(pc);
        s_current_main_fg = s_preview_is_running ? gcolor_legible_over(pc) : pc;
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
    bool hide_cond = s_is_activity_picking;
#if defined(PBL_COLOR)
    hide_cond = hide_cond || s_is_color_picking || s_is_intermediate_menu;
#endif
    if (s_action_bar && hide_cond) hide = true;

    bool hide_mid = hide || !is_detail_mode();

    if (s_dist_layer) layer_set_hidden(text_layer_get_layer(s_dist_layer), hide_mid);
    if (s_step_layer) layer_set_hidden(text_layer_get_layer(s_step_layer), hide_mid);
    
    if (!s_has_hr_sensor) {
        if (s_hr_layer) layer_set_hidden(text_layer_get_layer(s_hr_layer), true);
    } else {
        if (s_hr_layer) layer_set_hidden(text_layer_get_layer(s_hr_layer), hide_mid);
    }
    
    if (s_clock_layer) layer_set_hidden(text_layer_get_layer(s_clock_layer), hide_mid);

    if ((s_app_state < 3 || s_app_state >= 5) && !hide) {
        if (!s_is_custom_marquee) {
            trigger_marquee();
        }
    } else {
        if (!s_is_custom_marquee) {
            destroy_marquee_layers();
        }
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
    if (s_msg_layer) text_layer_set_text_color(s_msg_layer, s_current_main_fg);
    
    if (s_action_bar) {
        action_bar_layer_set_background_color(s_action_bar, pc);
        
        bool use_black_icons = gcolor_equal(gcolor_legible_over(pc), GColorBlack);
        load_action_icons(use_black_icons);
        
        bool icon_cond = s_is_activity_picking;
#if defined(PBL_COLOR)
        icon_cond = icon_cond || s_is_color_picking || s_is_intermediate_menu;
#endif
        if (s_action_bar && icon_cond) {
            action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_up);
#if defined(PBL_COLOR)
            if (s_is_intermediate_menu) {
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_check);
            } else {
                action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_save);
            }
#else
            action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_save);
#endif
            action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_down);
        } else
        {
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
        
        // テキストやグラフが背景・水平線の奥に隠れないように、全て手前に引き上げる
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
        if (s_msg_container_layer) layer_insert_above_sibling(s_msg_container_layer, s_mid_bg_layer);
    }

    if (s_mid_bg_layer) layer_mark_dirty(s_mid_bg_layer);
    if (s_graph_layer) layer_mark_dirty(s_graph_layer);
    if (s_msg_container_layer) layer_mark_dirty(s_msg_container_layer);
    
#if defined(PBL_COLOR)
    if (s_color_picker_layer) {
        layer_mark_dirty(s_color_picker_layer);
    }
#endif
}

/* ==========================================================
   通信コールバック＆タイマー関連
   ========================================================== */
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *t = dict_read_first(iterator);
    while (t) {
        if (t->key == MESSAGE_KEY_CMD) {
            int cmd = get_int(t);
            if (cmd == 10) {
                vibes_long_pulse();
            } else if (cmd == 11) {
                vibes_double_pulse();
            }
        }
        else if (t->key == MESSAGE_KEY_STATE) {
            uint8_t ps = (uint8_t)get_int(t);
            if (ps != s_app_state) {
#if defined(PBL_HEALTH)
                int total = (int)health_service_sum_today(HealthMetricStepCount);
#else
                int total = 0;
#endif
                if (s_app_state == 3 && ps == 4) {
                    s_pause_start_steps = total;
                } else if (s_app_state == 4 && ps == 3) {
                    s_start_steps += (total - s_pause_start_steps);
                    persist_write_int(PK_START_STEPS, s_start_steps);
                }
                if (ps == 1 && s_app_state != 1) clear_graph_data();
                
                s_app_state = ps;
                s_is_paused = (s_app_state != 3);
                bool menu_cond = s_is_activity_picking;
#if defined(PBL_COLOR)
                menu_cond = menu_cond || s_is_color_picking || s_is_intermediate_menu;
#endif
                if (s_action_bar && s_app_state >= 3 && menu_cond) {
                    s_is_activity_picking = false;
                    destroy_activity_picker_layer();
#if defined(PBL_COLOR)
                    s_is_color_picking = false;
                    s_is_intermediate_menu = false;
                    destroy_color_picker_layer();
                    destroy_intermediate_layer();
#endif
                    if (s_action_bar && s_main_window) {
                        action_bar_layer_remove_from_window(s_action_bar);
                        action_bar_layer_add_to_window(s_action_bar, s_main_window);
                    }
                }
                update_ui_state();
            }
        } 
        else if (t->key == MESSAGE_KEY_TIME) {
            char ts[16];
            snprintf(ts, 16, "%s", t->value->cstring);
            int c = 0;
            for (int i = 0; ts[i]; i++) {
                if (ts[i] == ':') c++;
            }
            bool old_long = s_is_long_workout;
            if (c == 1) {
                s_is_long_workout = false;
                char *p = strchr(ts, ':');
                if (p) {
                    int ml = p - ts;
                    if (ml == 1) snprintf(s_time_min_buf, 8, "0%c", ts[0]);
                    else snprintf(s_time_min_buf, 8, "%.*s", ml, ts);
                    snprintf(s_time_sec_buf, 8, "%s", p + 1);
                }
            } else if (c >= 2) {
                s_is_long_workout = true;
                char *p1 = strchr(ts, ':');
                char *p2 = strchr(p1 + 1, ':');
                if (p1 && p2) {
                    int hl = p1 - ts;
                    snprintf(s_time_hour_buf, 8, "%.*s", hl, ts);
                    snprintf(s_time_min_buf, 8, "%.*s", (int)(p2 - (p1 + 1)), p1 + 1);
                    snprintf(s_time_sec_buf, 8, "%s", p2 + 1);
                }
            }
            if (old_long != s_is_long_workout) update_ui_state();
            
            if (s_time_hour_layer) text_layer_set_text(s_time_hour_layer, s_time_hour_buf);
            if (s_time_min_layer) text_layer_set_text(s_time_min_layer, s_time_min_buf);
            if (s_time_sec_layer) text_layer_set_text(s_time_sec_layer, s_time_sec_buf);
        } 
        else if (t->key == MESSAGE_KEY_DISTANCE) {
            snprintf(s_dist_buf, 16, "%s", t->value->cstring);
            if (s_dist_layer) text_layer_set_text(s_dist_layer, s_dist_buf);
        }
        else if (t->key == MESSAGE_KEY_HR && s_has_hr_sensor) {
            snprintf(s_hr_buf, 16, "%s", t->value->cstring);
            if (s_hr_layer) text_layer_set_text(s_hr_layer, s_hr_buf);
        }
        else if (t->key == MESSAGE_KEY_GRAPH_DATA) {
            parse_hybrid_graph_data(t->value->cstring);
            save_graphs_to_persist();
            if (s_graph_layer) layer_mark_dirty(s_graph_layer);
        }
        else if (t->key == MESSAGE_KEY_MID_DATA) {
            parse_mid_data(t->value->cstring);
            update_ui_state();
        }
        else if (t->key == MESSAGE_KEY_ACTIVITY_TYPE) {
            int received_type = get_int(t);
            if(received_type >= 0 && received_type < ACTIVITY_COUNT) {
                s_current_activity = (ActivityType)received_type;
                persist_write_int(PK_ACTIVITY_TYPE, s_current_activity);
                if (s_is_activity_picking && s_activity_picker_layer) {
                    s_preview_activity_idx = s_current_activity;
                    layer_mark_dirty(s_activity_picker_layer);
                }
            }
        }
        t = dict_read_next(iterator);
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    static char last_clock_buf[16] = "";
    clock_copy_time_string(s_clock_buf, 16);
    if (strcmp(s_clock_buf, last_clock_buf) != 0) {
        strcpy(last_clock_buf, s_clock_buf);
        if (s_clock_layer) {
            text_layer_set_text(s_clock_layer, s_clock_buf);
        }
    }
    
#if defined(PBL_HEALTH)
    int total = (int)health_service_sum_today(HealthMetricStepCount);
#else
    int total = 0;
#endif
    
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
    static int last_hr = -1;
    if (s_has_hr_sensor) {
        s_current_hr = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
        if (s_current_hr != last_hr) {
            last_hr = s_current_hr;
            if (s_current_hr < 30) snprintf(s_hr_buf, 16, "--");
            else snprintf(s_hr_buf, 16, "%d", s_current_hr);
            if (s_hr_layer) text_layer_set_text(s_hr_layer, s_hr_buf);
        }
    } else {
        if (last_hr != -2) {
            snprintf(s_hr_buf, 16, "--");
            last_hr = -2;
        }
    }
#else
    snprintf(s_hr_buf, 16, "--");
#endif
    
#if defined(PBL_COLOR)
    if (s_is_color_picking && s_action_bar) {
        s_preview_is_running = !s_preview_is_running;
        update_ui_state();
    }
#endif
    
    int ds = (total - s_start_steps >= 0) ? total - s_start_steps : 0;
    if (s_app_state == 3 && tick_time->tm_sec % 5 == 0) {
        DictionaryIterator *it;
        if (app_message_outbox_begin(&it) == APP_MSG_OK) {
            dict_write_int32(it, KEY_HR, s_current_hr);
            dict_write_int32(it, KEY_STEPS, ds);
            app_message_outbox_send();
        }
    }
    
    static int last_ds = -1;
#if defined(PBL_HEALTH)
    if (ds != last_ds) {
        last_ds = ds;
        if (ds >= 10000) snprintf(s_step_buf, 16, "%d.%dK", ds / 1000, (ds % 1000) / 100);
        else snprintf(s_step_buf, 16, "%d", ds);
        if (s_step_layer) text_layer_set_text(s_step_layer, s_step_buf);
    }
#else
    if (last_ds != -2) {
        snprintf(s_step_buf, 16, "--");
        last_ds = -2;
        if (s_step_layer) text_layer_set_text(s_step_layer, s_step_buf);
    }
#endif
}

/* ==========================================================
   ウィンドウロード・メイン関数
   ========================================================== */

static void main_window_load(Window *window) {
    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
    
    int wt = b.size.w;
    int h = b.size.h;
    int active_w = wt - ACTION_BAR_WIDTH;

    s_is_small_screen = (wt <= 144);
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
    layer_set_update_proc(s_mid_bg_layer, mid_bg_update_proc);
    layer_add_child(wl, s_mid_bg_layer);
    
    int m3w, c3w, s3w, y3_base_m, y3_base_s, y3_colon, r_h3;
    int h5w, c1w, m5w, c2w, s5w, y5_base_h, y5_base_m, y5_base_s, y5_colon1, y5_colon2, r_h5;
    int offset_x3 = 0, offset_x5 = 0;
    int colon_x_offset = 0;
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
    // 枠幅はクリッピング防止のため余裕を持たせる (60px)
    m3w = 60; c3w = 14; s3w = 60;
    vm3 = 52; vc3 = 6; vs3 = 52; // mmとssの間隔を寄せる
    
    h5w = 26; c1w = 12; m5w = 60; c2w = 14; s5w = 60;
    vh5 = 28; vc1 = 0; vm5 = 52; vc2 = 6; vs5 = 52;
    
    r_h3 = 50; r_h5 = 50;
    mid_h = 45; upper_h = (h - mid_h) / 2; lower_h = h - mid_h - upper_h;
    
    y3_base_m = upper_h - 48; 
    y3_base_s = y3_base_m;
    y3_colon = y3_base_m + 12;
    
    y5_base_m = upper_h - 48; 
    y5_base_h = upper_h - 26; // hとmmの数字の下限フチを揃える
    y5_base_s = upper_h - 48; 
    y5_colon1 = y5_base_h + 2; // コロンを小さいフォントに合わせて高さ微調整
    y5_colon2 = y5_base_m + 12;
    offset_x3 = 11; // 左弧との干渉を避けるため右へ11px移動
    offset_x5 = -3; // 左コロンのみを右に3pxずらし、他は完全に合致させる
    
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
    // 枠幅は見切れないよう余裕を持たせる
    m3w = 80; c3w = 16; s3w = 80;
    vm3 = 68; vc3 = 8; vs3 = 68; // 実際の配置間隔（ここで中央に寄せています）
    
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
    offset_x5 = -6; // h:mm:ss を全体的に6ピクセル左へ移動
    
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
    y5_base_h = upper_h - 46; // h(38)とmm(60)の数字の下限フチを揃える
    y5_base_s = upper_h - 68; 
    y5_colon1 = y5_base_h + 10; // コロンを少し下に移動して安定させる
    y5_colon2 = y5_base_m + 22;
    offset_x3 = 15; // 左弧との干渉を避けるため右へ15px移動
    offset_x5 = -12; // 左コロンのみを右に3pxずらし、他は完全に合致させる
    
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
    layer_set_update_proc(s_graph_layer, graph_update_proc);
    layer_add_child(wl, s_graph_layer);

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
    
    // 共通レイヤー設定
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

    load_persist_data();
    update_ui_state(); 
    tick_handler(NULL, SECOND_UNIT); 
}

static void main_window_unload(Window *window) {
    destroy_marquee_layers();
    
    if (s_ignore_single_click_timer) {
        app_timer_cancel(s_ignore_single_click_timer);
        s_ignore_single_click_timer = NULL;
    }

    if (s_activity_picker_layer) {
        layer_destroy(s_activity_picker_layer);
        s_activity_picker_layer = NULL;
    }
#if defined(PBL_COLOR)
    if (s_color_picker_layer) {
        layer_destroy(s_color_picker_layer);
        s_color_picker_layer = NULL;
    }
    if (s_intermediate_layer) {
        layer_destroy(s_intermediate_layer);
        s_intermediate_layer = NULL;
    }
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

static void init() {
    app_message_register_inbox_received(inbox_received_callback);
    app_message_open(1024, 256);
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

static void deinit() {
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
