#pragma once

#include <pebble.h>

/* ==========================================================
   定数・マクロ・キー定義
   ========================================================== */

#define KEY_STEPS 10010
#define KEY_HR 10007
#define KEY_HR_INTERVAL 10014
#define MESSAGE_KEY_CMD 10000
#define MESSAGE_KEY_TIME 10001
#define MESSAGE_KEY_DISTANCE 10002
#define MESSAGE_KEY_PACE 10003
#define MESSAGE_KEY_STATE 10004
#define MESSAGE_KEY_MEDIA_CMD 10008  
#define MESSAGE_KEY_GRAPH_DATA 10009
#define MESSAGE_KEY_MID_DATA 10013
#define MESSAGE_KEY_ACTIVITY_TYPE 10012

// 新規拡張メッセージキー (ID指定および下段データ)
#define MESSAGE_KEY_MID_ID 10015
#define MESSAGE_KEY_LOWER_ID 10016
#define MESSAGE_KEY_LOWER_DATA 10017
#define MESSAGE_KEY_KEY_EVENT 10018
#define MESSAGE_KEY_MAP_DATA 10019
#define MESSAGE_KEY_MAP_CHUNK_IDX 10020
#define MESSAGE_KEY_MAP_TOTAL_CHUNKS 10021
#define MESSAGE_KEY_MAP_STATE 10022

typedef enum {
    EVENT_NONE = 0,
    // ボタン短押し
    EVENT_BUTTON_UP_CLICK = 1,
    EVENT_BUTTON_SELECT_CLICK = 2,
    EVENT_BUTTON_DOWN_CLICK = 3,
    
    // ボタン長押し
    EVENT_BUTTON_UP_LONG = 11,
    EVENT_BUTTON_SELECT_LONG = 12,
    EVENT_BUTTON_DOWN_LONG = 13,
    
    // タッチ操作（Emery/Chalk）
    EVENT_TOUCH_DOUBLE_TAP = 21,
    EVENT_TOUCH_SWIPE_LEFT = 22,
    EVENT_TOUCH_SWIPE_RIGHT = 23,
    EVENT_TOUCH_SWIPE_UP = 24,
    EVENT_TOUCH_SWIPE_DOWN = 25
} AppEventID;

#define PK_GRAPH_COUNT 53
#define PK_GRAPH_DATA 54
#define PK_GRAPH_ID 55
#define PK_GRAPH_SCALE 56
#define PK_PERSONAL_COLOR 57 
#define PK_ACTIVITY_TYPE 59
#define PK_LAST_MID_ID 60
#define PK_LAST_LOWER_ID 61

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

extern const char* const ACTIVITY_NAMES[ACTIVITY_COUNT];

#define MAX_MID_PAGES 15
#define MAX_LOWER_PAGES 15

typedef struct {
    int id;            // 固有ID (1〜99: テキスト項目, 100〜: グラフ項目, 0: DETAIL)
    char name[16];
    char value[16];
    char unit[16];
    int icon_id;
} MetricPageData;

typedef MetricPageData MidPageData;
typedef MetricPageData LowerPageData;

#define MAX_GRAPH_DATA 45

#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_CHALK)
#define SWIPE_MIN_DIST_PX 30      
#define SWIPE_MAX_TIME_MS 800
#define DOUBLE_TAP_MAX_DELAY_MS 500
#define TAP_MAX_DIST_PX 15
#endif

// ユーティリティ
static inline uint64_t app_get_current_time_ms(void) {
    time_t s;
    uint16_t ms;
    time_ms(&s, &ms);
    return ((uint64_t)s * 1000) + ms;
}

static inline int32_t app_get_int_from_tuple(Tuple *t) {
    if (!t) return 0;
    if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
    if (t->length == 1) return (int32_t)t->value->uint8;
    if (t->length == 2) return (int32_t)t->value->uint16;
    if (t->length == 4) return (int32_t)t->value->uint32;
    return 0;
}

static inline void app_extract_token(const char **ptr, char *out, int max_len) {
    int i = 0;
    // 前後の空白をスキップ
    while (**ptr == ' ') (*ptr)++;
    while (**ptr && **ptr != ',' && **ptr != '|') {
        if (i < max_len - 1) out[i++] = **ptr;
        (*ptr)++;
    }
    out[i] = '\0';
    if (**ptr == ',' || **ptr == '|') (*ptr)++;
}

static inline bool app_is_numeric_string(const char *str) {
    if (!str || *str == '\0') return false;
    while (*str) {
        if (*str < '0' || *str > '9') return false;
        str++;
    }
    return true;
}
