#pragma once
#include "../app_state.h"
#include "graph_data.h"

typedef void (*CommServiceUIUpdateCallback)(void);
typedef void (*CommServiceGraphDirtyCallback)(void);

void comm_service_init(CommServiceUIUpdateCallback ui_update_cb, CommServiceGraphDirtyCallback graph_dirty_cb);
void comm_service_send_cmd(int val);
void comm_service_send_map_state(int state);
bool comm_service_is_map_open_requested(void);
void comm_service_clear_map_open_request(void);
void comm_service_send_button_event(AppEventID event_id, int legacy_cmd);
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
void comm_service_send_media_event(AppEventID event_id, int legacy_media_cmd);
#endif
void comm_service_send_activity_type(ActivityType type);
void comm_service_send_mid_id(int mid_id);
void comm_service_send_lower_id(int lower_id);
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
void comm_service_send_media_cmd(int cmd);
#endif
void comm_service_send_health_data(bool send_steps, bool send_hr, int steps, int hr);
int32_t comm_service_get_hr_interval_setting(void);

void comm_service_set_ui_buffers(
    char *time_hour_buf, char *time_min_buf, char *time_sec_buf,
    char *dist_buf, char *hr_buf, char *step_buf,
    TextLayer *dist_layer, TextLayer *hr_layer, TextLayer *step_layer,
    uint8_t *app_state_ptr, bool *is_paused_ptr, bool *is_long_workout_ptr,
    ActivityType *current_activity_ptr, bool has_hr_sensor
);
