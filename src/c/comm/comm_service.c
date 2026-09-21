#include "../ui/ui_map.h"
#include "../ui/ui_course_picker.h"
#include "comm_service.h"

static CommServiceUIUpdateCallback s_ui_update_cb = NULL;
static CommServiceGraphDirtyCallback s_graph_dirty_cb = NULL;

static char *s_time_hour_buf = NULL;
static char *s_time_min_buf = NULL;
static char *s_time_sec_buf = NULL;
static char *s_dist_buf = NULL;
static char *s_hr_buf = NULL;
static char *s_step_buf = NULL;
static TextLayer *s_dist_layer = NULL;
static TextLayer *s_hr_layer = NULL;
static TextLayer *s_step_layer = NULL;
static uint8_t *s_app_state_ptr = NULL;
static bool *s_is_paused_ptr = NULL;
static bool *s_is_long_workout_ptr = NULL;
static ActivityType *s_current_activity_ptr = NULL;
static bool s_has_hr_sensor = false;
static int32_t s_hr_interval_setting = 0;
static bool s_map_open_requested = false;


static uint32_t s_current_elapsed_seconds = 0;
static bool s_map_transfer_in_progress = false;

static inline void format_2digits(char *buf, uint32_t val) {
    buf[0] = '0' + (char)((val / 10) % 10);
    buf[1] = '0' + (char)(val % 10);
    buf[2] = '\0';
}

static uint32_t parse_formatted_time(const char *raw) {
    if (!raw) return 0;
    uint32_t v1 = 0, v2 = 0, v3 = 0;
    int colons = 0;
    const char *p = raw;
    while (*p) {
        if (*p == ':') {
            colons++;
        } else if (*p >= '0' && *p <= '9') {
            if (colons == 0) v1 = v1 * 10 + (uint32_t)(*p - '0');
            else if (colons == 1) v2 = v2 * 10 + (uint32_t)(*p - '0');
            else if (colons == 2) v3 = v3 * 10 + (uint32_t)(*p - '0');
        }
        p++;
    }
    if (colons >= 2) {
        return v1 * 3600 + v2 * 60 + v3;
    } else if (colons == 1) {
        return v1 * 60 + v2;
    }
    return 0;
}

void comm_service_format_time_to_buffers(uint32_t total_sec) {
    uint32_t h = total_sec / 3600;
    uint32_t m = (total_sec % 3600) / 60;
    uint32_t s = total_sec % 60;

    if (h > 0) {
        if (s_is_long_workout_ptr) *s_is_long_workout_ptr = true;
        if (s_time_hour_buf) {
            if (h >= 10) {
                format_2digits(s_time_hour_buf, h);
            } else {
                s_time_hour_buf[0] = '0' + (char)h;
                s_time_hour_buf[1] = '\0';
            }
        }
        if (s_time_min_buf) format_2digits(s_time_min_buf, m);
        if (s_time_sec_buf) format_2digits(s_time_sec_buf, s);
    } else {
        if (s_is_long_workout_ptr) *s_is_long_workout_ptr = false;
        if (s_time_hour_buf) s_time_hour_buf[0] = '\0';
        if (s_time_min_buf) format_2digits(s_time_min_buf, m);
        if (s_time_sec_buf) format_2digits(s_time_sec_buf, s);
    }
}

bool comm_service_is_map_transfer_in_progress(void) {
    return s_map_transfer_in_progress;
}

void comm_service_increment_elapsed_seconds(void) {
    s_current_elapsed_seconds++;
    comm_service_format_time_to_buffers(s_current_elapsed_seconds);
}

uint32_t comm_service_get_elapsed_seconds(void) {
    return s_current_elapsed_seconds;
}

void comm_service_reset_elapsed_seconds(void) {
    s_current_elapsed_seconds = 0;
    comm_service_format_time_to_buffers(0);
}

bool comm_service_is_map_open_requested(void) { return s_map_open_requested; }
void comm_service_clear_map_open_request(void) { s_map_open_requested = false; }

void comm_service_set_ui_buffers(
    char *time_hour_buf, char *time_min_buf, char *time_sec_buf,
    char *dist_buf, char *hr_buf, char *step_buf,
    TextLayer *dist_layer, TextLayer *hr_layer, TextLayer *step_layer,
    uint8_t *app_state_ptr, bool *is_paused_ptr, bool *is_long_workout_ptr,
    ActivityType *current_activity_ptr, bool has_hr_sensor
) {
    s_time_hour_buf = time_hour_buf;
    s_time_min_buf = time_min_buf;
    s_time_sec_buf = time_sec_buf;
    s_dist_buf = dist_buf;
    s_hr_buf = hr_buf;
    s_step_buf = step_buf;
    s_dist_layer = dist_layer;
    s_hr_layer = hr_layer;
    s_step_layer = step_layer;
    s_app_state_ptr = app_state_ptr;
    s_is_paused_ptr = is_paused_ptr;
    s_is_long_workout_ptr = is_long_workout_ptr;
    s_current_activity_ptr = current_activity_ptr;
    s_has_hr_sensor = has_hr_sensor;
}

void comm_service_send_button_event(AppEventID event_id) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_KEY_EVENT, (int32_t)event_id);
        app_message_outbox_send();
    }
}

void comm_service_send_map_state(int state) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_MAP_STATE, state);
        app_message_outbox_send();
    }
}

void comm_service_send_activity_type(ActivityType type) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_ACTIVITY_TYPE, (int32_t)type);
        app_message_outbox_send();
    }
}

void comm_service_send_mid_id(int mid_id) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_MID_ID, mid_id);
        app_message_outbox_send();
    }
}

void comm_service_send_lower_id(int lower_id) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_LOWER_ID, lower_id);
        app_message_outbox_send();
    }
}

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_GABBRO)
void comm_service_send_media_event(AppEventID event_id) {
    comm_service_send_button_event(event_id);
}

void comm_service_send_touch_pan(int16_t dx, int16_t dy) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_KEY_PAN_DX, (int32_t)dx);
        dict_write_int32(iter, MESSAGE_KEY_KEY_PAN_DY, (int32_t)dy);
        app_message_outbox_send();
    }
}
#endif

void comm_service_send_health_data(bool send_steps, bool send_hr, int steps, int hr) {
    DictionaryIterator *it;
    if ((send_steps || send_hr) && app_message_outbox_begin(&it) == APP_MSG_OK) {
        if (send_steps) dict_write_int32(it, KEY_STEPS, steps);
        if (send_hr) dict_write_int32(it, KEY_HR, hr);
        app_message_outbox_send();
    }
}

int32_t comm_service_get_hr_interval_setting(void) {
    return s_hr_interval_setting;
}

void comm_service_request_sync(void) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_int32(iter, MESSAGE_KEY_CMD, 5);
        app_message_outbox_send();
    }
}

void comm_service_send_course_toggle(bool is_enabled, const char *course_name) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d,%s", is_enabled ? 1 : 0, course_name ? course_name : "");
        dict_write_cstring(iter, MESSAGE_KEY_KEY_COURSES_DATA, buf);
        app_message_outbox_send();
    }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *t = dict_read_first(iterator);
    bool should_update_ui = false;

    while (t != NULL) {
        if (t->key == MESSAGE_KEY_CMD) {
            int cmd = (int)app_get_int_from_tuple(t);
            if (cmd == 10) {
                vibes_long_pulse();
            } else if (cmd == 11) {
                vibes_double_pulse();
            }
        }
        else if (t->key == MESSAGE_KEY_STATE && s_app_state_ptr) {
            uint8_t new_state = (uint8_t)app_get_int_from_tuple(t);
            if (*s_app_state_ptr != new_state) {
                *s_app_state_ptr = new_state;
                if (s_is_paused_ptr) *s_is_paused_ptr = (*s_app_state_ptr == 4 || *s_app_state_ptr == 5 || *s_app_state_ptr == 6);
                if (new_state == 0 || new_state == 1 || new_state == 2) {
                    comm_service_reset_elapsed_seconds();
                    s_map_transfer_in_progress = false;
                }
                should_update_ui = true;
            }
        }
        else if (t->key == MESSAGE_KEY_TIME) {
            const char *raw = t->value->cstring;
            if (raw && strlen(raw) >= 5 && s_time_min_buf && s_time_sec_buf) {
                uint32_t rx_sec = parse_formatted_time(raw);
                uint8_t state = s_app_state_ptr ? *s_app_state_ptr : 0;
                
                // 逆行防止ガード:
                // 計測中(3)の場合、スマホ時刻が表示秒数以上の時のみ受け入れる。
                // （過去パケットや遅延パケットで時計が戻る現象を完全排除）
                // ただし、状態が計測中以外、またはスマホ時刻との差が5秒以上（リセットや再開等）の場合は強制同期
                if (state != 3 || rx_sec >= s_current_elapsed_seconds || (s_current_elapsed_seconds - rx_sec > 5)) {
                    s_current_elapsed_seconds = rx_sec;
                    comm_service_format_time_to_buffers(s_current_elapsed_seconds);
                    should_update_ui = true;
                }
            }
        }
        else if (t->key == MESSAGE_KEY_DISTANCE) {
            if (s_dist_buf) {
                snprintf(s_dist_buf, 16, "%s", t->value->cstring);
                if (s_dist_layer) text_layer_set_text(s_dist_layer, s_dist_buf);
            }
        }
        else if (t->key == KEY_STEPS) {
            if (s_step_buf) {
                if (t->type == TUPLE_CSTRING) {
                    snprintf(s_step_buf, 16, "%s", t->value->cstring);
                } else {
                    int ds = app_get_int_from_tuple(t);
                    if (ds >= 10000) snprintf(s_step_buf, 16, "%d.%dK", ds / 1000, (ds % 1000) / 100);
                    else snprintf(s_step_buf, 16, "%d", ds);
                }
                if (s_step_layer) text_layer_set_text(s_step_layer, s_step_buf);
            }
        }
        else if (t->key == MESSAGE_KEY_HR && s_has_hr_sensor) {
            if (s_hr_buf) {
                snprintf(s_hr_buf, 16, "%s", t->value->cstring);
                if (s_hr_layer) text_layer_set_text(s_hr_layer, s_hr_buf);
            }
        }
        else if (t->key == MESSAGE_KEY_GRAPH_DATA) {
            graph_data_parse_hybrid(t->value->cstring);
            graph_data_save_to_persist();
            if (s_graph_dirty_cb) s_graph_dirty_cb();
        }
        else if (t->key == MESSAGE_KEY_MID_DATA) {
            graph_data_parse_mid(t->value->cstring);
            should_update_ui = true;
        }
        else if (t->key == MESSAGE_KEY_LOWER_DATA) {
            graph_data_parse_lower(t->value->cstring);
            should_update_ui = true;
        }
        else if (t->key == MESSAGE_KEY_MID_ID) {
            int target_id = (int)app_get_int_from_tuple(t);
            graph_data_select_mid_by_id(target_id);
            should_update_ui = true;
        }
        else if (t->key == MESSAGE_KEY_LOWER_ID) {
            int target_id = (int)app_get_int_from_tuple(t);
            graph_data_select_lower_by_id(target_id);
            should_update_ui = true;
        }
        else if (t->key == MESSAGE_KEY_ACTIVITY_TYPE && s_current_activity_ptr) {
            int received_type = app_get_int_from_tuple(t);
            if(received_type >= 0 && received_type < ACTIVITY_COUNT) {
                *s_current_activity_ptr = (ActivityType)received_type;
                persist_write_int(PK_ACTIVITY_TYPE, *s_current_activity_ptr);
                should_update_ui = true;
            }
        }
        else if (t->key == MESSAGE_KEY_MAP_DATA) {
            Tuple *t_idx = dict_find(iterator, MESSAGE_KEY_MAP_CHUNK_IDX);
            Tuple *t_total = dict_find(iterator, MESSAGE_KEY_MAP_TOTAL_CHUNKS);
            int c_idx = t_idx ? (int)app_get_int_from_tuple(t_idx) : 0;
            int c_total = t_total ? (int)app_get_int_from_tuple(t_total) : 1;
            ui_map_update_data(t->value->data, t->length, c_idx, c_total);
            if (c_idx == 0) {
                s_map_transfer_in_progress = true;
            }
            if (c_idx == c_total - 1) {
                s_map_transfer_in_progress = false;
            }
            should_update_ui = true;
        }
        else if (t->key == MESSAGE_KEY_KEY_COURSES_DATA) {
            const char *raw_courses = t->value->cstring;
            if (raw_courses) {
                ui_course_picker_parse_and_set(raw_courses);
                should_update_ui = true;
            }
        }
        else if (t->key == MESSAGE_KEY_MAP_STATE) {
            int map_state = (int)app_get_int_from_tuple(t);
            if (map_state == 1 && !ui_map_is_active()) {
                s_map_open_requested = true;
                should_update_ui = true;
            } else if (map_state == 0 && ui_map_is_active()) {
                s_map_open_requested = false;
                ui_map_destroy();
                should_update_ui = true;
            }
        }
        else if (t->key == KEY_HR_INTERVAL) {
#if defined(PBL_HEALTH)
            if (s_has_hr_sensor) {
                s_hr_interval_setting = app_get_int_from_tuple(t);
            }
#endif
        }
        t = dict_read_next(iterator);
    }

    if (should_update_ui && s_ui_update_cb) {
        s_ui_update_cb();
    }
}

void comm_service_init(CommServiceUIUpdateCallback ui_update_cb, CommServiceGraphDirtyCallback graph_dirty_cb) {
    s_ui_update_cb = ui_update_cb;
    s_graph_dirty_cb = graph_dirty_cb;
    app_message_register_inbox_received(inbox_received_callback);
    app_message_open(1024, 256);
}
