#include "ui_mic_visualizer.h"

#if defined(_PBL_API_EXISTS_mic_capture_start)

#define MIC_TIMEOUT_MS 10000

static Layer *s_visualizer_layer = NULL;
static Window *s_target_window = NULL;
static AppTimer *s_timeout_timer = NULL;
static int s_current_level = 0; // 0 - 100
static int s_anim_phase = 0;

static void prv_timeout_timer_handler(void *context) {
    s_timeout_timer = NULL;
    APP_LOG(APP_LOG_LEVEL_INFO, "Mic visualizer timed out after 10s.");
    ui_mic_visualizer_stop();
}

static void prv_mic_data_handler(const int16_t *samples, size_t sample_count, void *context) {
    if (!samples || sample_count == 0) return;

    int16_t max_peak = 0;
    for (size_t i = 0; i < sample_count; i++) {
        int16_t val = samples[i];
        if (val < 0) {
            val = (val == -32768) ? 32767 : -val;
        }
        if (val > max_peak) {
            max_peak = val;
        }
    }

    // 0〜14000 を 0〜100 にスケール
    int target_level = ((int32_t)max_peak * 100) / 14000;
    if (target_level > 100) target_level = 100;

    // スムージング（急激な下降を抑えて波形の動きを滑らかに）
    if (target_level > s_current_level) {
        s_current_level = target_level;
    } else {
        s_current_level = (s_current_level * 3 + target_level) / 4;
    }

    s_anim_phase = (s_anim_phase + 1) % 8;

    if (s_visualizer_layer) {
        layer_mark_dirty(s_visualizer_layer);
    }
}

static void prv_mic_stopped_handler(MicCaptureStopReason reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Mic capture stopped involuntarily: %d", (int)reason);
    ui_mic_visualizer_stop();
}

static void prv_visualizer_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // 背景を暗く塗りつぶす（オーバーレイ領域）
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // 上部に境界線を描画
#if defined(PBL_COLOR)
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
    graphics_draw_line(ctx, GPoint(bounds.origin.x, bounds.origin.y), GPoint(bounds.origin.x + bounds.size.w, bounds.origin.y));

    // ギザギザ波形の描画
#if defined(PBL_COLOR)
    // カラー画面ではサイバーな蛍光グリーン（SpringBud）
    graphics_context_set_stroke_color(ctx, GColorSpringBud);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
#endif

    int mid_y = bounds.origin.y + (bounds.size.h / 2);
    // 振幅（最小1px〜最大 (bounds.size.h/2 - 2)px）
    int max_amp = (bounds.size.h / 2) - 2;
    int amp = 1 + (s_current_level * max_amp) / 100;

    int step_x = 5;
    int x_start = bounds.origin.x;
    int x_end = bounds.origin.x + bounds.size.w;

    GPoint prev_pt = GPoint(x_start, mid_y);

    for (int x = x_start + step_x; x <= x_end; x += step_x) {
        // 位相によるゆらぎ
        int wave_offset = (((x - x_start) / step_x) + s_anim_phase) % 2 == 0 ? 1 : -1;
        int y = mid_y + (wave_offset * amp);

        GPoint next_pt = GPoint(x, y);
        graphics_draw_line(ctx, prev_pt, next_pt);
        prev_pt = next_pt;
    }
}

void ui_mic_visualizer_start(Window *window) {
    if (ui_mic_visualizer_is_active()) {
        // 既に起動中の場合は10秒タイムアウトを延長（再トリガー対応）
        if (s_timeout_timer) {
            app_timer_cancel(s_timeout_timer);
        }
        s_timeout_timer = app_timer_register(MIC_TIMEOUT_MS, prv_timeout_timer_handler, NULL);
        return;
    }
    if (!window) {
        window = window_stack_get_top_window();
    }
    if (!window) return;

    s_target_window = window;
    s_current_level = 0;
    s_anim_phase = 0;

    MicCaptureHandlers handlers = {
        .data = prv_mic_data_handler,
        .stopped = prv_mic_stopped_handler,
    };

    MicCaptureStartResult result = mic_capture_start(handlers, NULL);
    if (result != MicCaptureStartOk) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "mic_capture_start failed with code: %d", (int)result);
        return;
    }

    Layer *root_layer = window_get_root_layer(window);
    GRect root_bounds = layer_get_bounds(root_layer);

    int bar_height = 16;
    GRect vis_rect;

#if defined(PBL_ROUND)
    // 丸画面（Gabbro）では最下部の湾曲を避けて少し内側に配置
    vis_rect = GRect(24, root_bounds.size.h - bar_height - 6, root_bounds.size.w - 48, bar_height);
#else
    // 四角画面（Emery / Flint）では画面最下部にフィット
    vis_rect = GRect(0, root_bounds.size.h - bar_height, root_bounds.size.w, bar_height);
#endif

    s_visualizer_layer = layer_create(vis_rect);
    layer_set_update_proc(s_visualizer_layer, prv_visualizer_update_proc);
    layer_add_child(root_layer, s_visualizer_layer);

    // 10秒自動タイムアウトタイマーの登録
    if (s_timeout_timer) {
        app_timer_cancel(s_timeout_timer);
    }
    s_timeout_timer = app_timer_register(MIC_TIMEOUT_MS, prv_timeout_timer_handler, NULL);

    vibes_short_pulse();
    APP_LOG(APP_LOG_LEVEL_INFO, "Mic visualizer started successfully!");
}

void ui_mic_visualizer_stop(void) {
    if (!ui_mic_visualizer_is_active()) return;

    if (s_timeout_timer) {
        app_timer_cancel(s_timeout_timer);
        s_timeout_timer = NULL;
    }

    if (mic_capture_is_active()) {
        mic_capture_stop();
    }

    if (s_visualizer_layer) {
        layer_remove_from_parent(s_visualizer_layer);
        layer_destroy(s_visualizer_layer);
        s_visualizer_layer = NULL;
    }

    s_target_window = NULL;
    s_current_level = 0;

    APP_LOG(APP_LOG_LEVEL_INFO, "Mic visualizer stopped.");
}

bool ui_mic_visualizer_is_active(void) {
    return s_visualizer_layer != NULL;
}

#endif
