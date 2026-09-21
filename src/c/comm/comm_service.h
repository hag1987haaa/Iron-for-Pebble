#pragma once
#include "../app_state.h"
#include "graph_data.h"

typedef void (*CommServiceUIUpdateCallback)(void);
typedef void (*CommServiceGraphDirtyCallback)(void);

void comm_service_init(CommServiceUIUpdateCallback ui_update_cb, CommServiceGraphDirtyCallback graph_dirty_cb);
void comm_service_send_map_state(int state);
bool comm_service_is_map_open_requested(void);
void comm_service_clear_map_open_request(void);
void comm_service_send_button_event(AppEventID event_id);
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_GABBRO)
void comm_service_send_media_event(AppEventID event_id);
void comm_service_send_touch_pan(int16_t dx, int16_t dy);
#endif
void comm_service_send_activity_type(ActivityType type);
void comm_service_send_mid_id(int mid_id);
void comm_service_send_lower_id(int lower_id);
void comm_service_send_health_data(bool send_steps, bool send_hr, int steps, int hr);
int32_t comm_service_get_hr_interval_setting(void);
void comm_service_request_sync(void);
void comm_service_send_course_toggle(bool is_enabled, const char *course_name);


bool comm_service_is_map_transfer_in_progress(void);
void comm_service_increment_elapsed_seconds(void);
uint32_t comm_service_get_elapsed_seconds(void);
void comm_service_reset_elapsed_seconds(void);
void comm_service_format_time_to_buffers(uint32_t total_sec);

void comm_service_set_ui_buffers(
    char *time_hour_buf, char *time_min_buf, char *time_sec_buf,
    char *dist_buf, char *hr_buf, char *step_buf,
    TextLayer *dist_layer, TextLayer *hr_layer, TextLayer *step_layer,
    uint8_t *app_state_ptr, bool *is_paused_ptr, bool *is_long_workout_ptr,
    ActivityType *current_activity_ptr, bool has_hr_sensor
);
