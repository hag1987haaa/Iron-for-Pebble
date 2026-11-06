#pragma once
#include <pebble.h>

#if defined(_PBL_API_EXISTS_mic_capture_start)

void ui_mic_visualizer_start(Window *window);
void ui_mic_visualizer_stop(void);
bool ui_mic_visualizer_is_active(void);

#else

// マイクAPI非対応機種（Aplite, Basalt, Chalk, Diorite等）向けのダミー関数
static inline void ui_mic_visualizer_start(Window *window) {}
static inline void ui_mic_visualizer_stop(void) {}
static inline bool ui_mic_visualizer_is_active(void) { return false; }

#endif
