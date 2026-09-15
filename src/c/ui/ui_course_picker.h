#pragma once

#include <pebble.h>
#include "../app_state.h"

// コースデータの初期化・Persistent Storage読み込み
void ui_course_picker_init(void);

// コースデータのPersistent Storage保存
void ui_course_picker_save(void);

// UI作成・破棄・状態判定
void ui_course_picker_create(Window *window, ActionBarLayer *action_bar, GColor main_bg, GColor main_fg);
void ui_course_picker_destroy(void);
bool ui_course_picker_is_active(void);

// 操作ハンドラ
void ui_course_picker_handle_up(void);
void ui_course_picker_handle_down(void);
void ui_course_picker_handle_select(void);

// コース一覧の取得・設定
int ui_course_picker_get_count(void);
const CourseItem* ui_course_picker_get_courses(void);
void ui_course_picker_set_course(int idx, int id, const char *name, bool is_enabled);

// Androidからの一括区切り文字列("1,NAME1|0,NAME2...")のパース＆一括保存
void ui_course_picker_parse_and_set(const char *data_str);
