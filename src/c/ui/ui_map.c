#include "ui_map.h"

#if !defined(PBL_PLATFORM_APLITE)

static Layer *s_map_layer = NULL;
static bool s_is_map_active = false;
static GColor s_current_bg, s_current_fg;

// RLEマップデータバッファ
#if defined(PBL_PLATFORM_APLITE)
#define MAX_MAP_RLE_SIZE 32
#elif defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
// 大画面（Emery 200x176 / Gabbro 260x198）の街中高密度マップでも100%溢れない16KBバッファ（最大32チャンク）
#define MAX_MAP_RLE_SIZE 16384
#else
#define MAX_MAP_RLE_SIZE 8192
#endif
#define MAP_CHUNK_SIZE 500
#define MAP_HEADER_MAGIC0 0xFE
#define MAP_HEADER_MAGIC1 0xFD
static uint8_t s_map_rle_data[MAX_MAP_RLE_SIZE];
static int s_map_rle_len = 0;
static bool s_has_received_data = false;
static bool s_transfer_started_from_zero = false;
static uint32_t s_received_chunks_mask = 0;
static int s_expected_total_chunks = 0;

// 時計・タイマーバッファポインタ
static const char *s_clock_ptr = NULL;
static const char *s_hour_ptr = NULL;
static const char *s_min_ptr = NULL;
static const char *s_sec_ptr = NULL;
static const bool *s_is_long_workout_ptr = NULL;

void ui_map_init_buffers(const char *clock_buf, const char *h_buf, const char *m_buf, const char *s_buf, const bool *is_long_workout) {
    s_clock_ptr = clock_buf;
    s_hour_ptr = h_buf;
    s_min_ptr = m_buf;
    s_sec_ptr = s_buf;
    s_is_long_workout_ptr = is_long_workout;
}

// RLEストリーム描画処理
static void draw_rle_map(GContext *ctx, GRect map_rect) {
    // 背景クリア（黒背景）
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, map_rect, 0, GCornerNone);

    int map_w = map_rect.size.w;
    int map_h = map_rect.size.h;

    // 受信済みRLEデータがある場合はプログレッシブに走査線を描画
    if (s_has_received_data && s_map_rle_len >= 2) {
        int start_idx = 0;
        int src_w = map_w;

        // 4バイトヘッダ [0xFE, 0xFD, width_low, height_low] がある場合は、
        // Android側がエンコードした正確な横幅(src_w)を取得して走査線ズレ（斜め砂嵐）を完全防止
        if (s_map_rle_len >= 6 &&
            s_map_rle_data[0] == MAP_HEADER_MAGIC0 &&
            s_map_rle_data[1] == MAP_HEADER_MAGIC1) {
            int encoded_w = s_map_rle_data[2];
            if (encoded_w == 4) {
                encoded_w = 260; // Gabbro (260 & 0xFF == 4)
            }
            if (encoded_w >= 100 && encoded_w <= 300) {
                src_w = encoded_w;
            }
            start_idx = 4;
        }

        int cur_x = 0;
        int cur_y = 0;
        // 画面幅とエンコード幅が異なる場合でも中央寄せで崩れずに描画
        int x_offset = (map_w > src_w) ? ((map_w - src_w) / 2) : 0;
        // 必ず2バイト（[count, color]ペア）境界でループを終端
        int safe_end = start_idx + (((s_map_rle_len - start_idx) / 2) * 2);

        for (int i = start_idx; i < safe_end; i += 2) {
            int count = s_map_rle_data[i];
            uint8_t color_val = s_map_rle_data[i + 1];
            if (count <= 0) continue;
            
            GColor c;
#if defined(PBL_COLOR)
            c = (GColor){.argb = color_val};
#else
            GColor orig = (GColor){.argb = color_val};
            c = gcolor_equal(gcolor_legible_over(orig), GColorBlack) ? GColorWhite : GColorBlack;
#endif
            graphics_context_set_stroke_color(ctx, c);
            graphics_context_set_stroke_width(ctx, 1);

            while (count > 0 && cur_y < map_h) {
                int line_remain = src_w - cur_x;
                int draw_len = (count < line_remain) ? count : line_remain;
                
                int draw_x0 = x_offset + cur_x;
                int draw_x1 = draw_x0 + draw_len - 1;
                if (draw_x0 < map_w) {
                    if (draw_x1 >= map_w) draw_x1 = map_w - 1;
                    graphics_draw_line(ctx, 
                        GPoint(draw_x0, cur_y), 
                        GPoint(draw_x1, cur_y)
                    );
                }
                
                cur_x += draw_len;
                count -= draw_len;
                
                if (cur_x >= src_w) {
                    cur_x = 0;
                    cur_y++;
                }
            }
            if (cur_y >= map_h) break;
        }
    }

    // 画面中央の現在地マーカーはAndroid側でアローとして描画するため省略
}

// レイヤー描画プロシージャ
static void map_update_proc(Layer *layer, GContext *ctx) {
    if (!s_is_map_active) return;
    GRect b = layer_get_bounds(layer);
    int wt = b.size.w;
    int h = b.size.h;

    // プラットフォームに応じた下部パネル高さの計算
    int panel_h = 40;
#if defined(PBL_PLATFORM_EMERY)
    panel_h = 52;
#elif defined(PBL_PLATFORM_CHALK)
    panel_h = 44;
#elif defined(PBL_PLATFORM_GABBRO)
    panel_h = 62;
#endif

    int map_h = h - panel_h;
    GRect map_rect = GRect(0, 0, wt, map_h);

    // 1. マップ画像および現在地マーカーの描画
    draw_rle_map(ctx, map_rect);

    // 2. 下部情報パネルの描画（黒背景＋境界線）
    GRect panel_rect = GRect(0, map_h, wt, panel_h);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, panel_rect, 0, GCornerNone);

    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(0, map_h), GPoint(wt, map_h));

    // 下部情報：タイム文字列の生成
    char time_str[32] = "";
    bool is_long = s_is_long_workout_ptr ? *s_is_long_workout_ptr : false;
    const char *h_buf = s_hour_ptr ? s_hour_ptr : "";
    const char *m_buf = s_min_ptr ? s_min_ptr : "00";
    const char *s_sec = s_sec_ptr ? s_sec_ptr : "00";

    if (is_long && strlen(h_buf) > 0) {
        snprintf(time_str, sizeof(time_str), "%s:%s:%s", h_buf, m_buf, s_sec);
    } else {
        snprintf(time_str, sizeof(time_str), "%s:%s", m_buf, s_sec);
    }

    // 中段連動データ（ラベル・値・単位）の取得
    const MidPageData *mid_page = graph_data_get_current_mid_page();
    const char *mid_name = "DIST";
    char mid_val_unit[48] = "--";
    if (mid_page) {
        if (strlen(mid_page->name) > 0) {
            mid_name = mid_page->name;
        }
        if (strlen(mid_page->unit) > 0) {
            snprintf(mid_val_unit, sizeof(mid_val_unit), "%s %s", mid_page->value, mid_page->unit);
        } else {
            snprintf(mid_val_unit, sizeof(mid_val_unit), "%s", mid_page->value);
        }
    }

    // フォントの統一（左右を同じGothic Boldフォントに統一）
    GFont data_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    GFont label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

#if defined(PBL_PLATFORM_EMERY)
    data_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
#elif defined(PBL_PLATFORM_GABBRO)
    data_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
#elif defined(PBL_PLATFORM_CHALK)
    data_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    label_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
#endif

    // ラベルもコントラスト確保のため「白」に設定
    graphics_context_set_text_color(ctx, GColorWhite);

#if defined(PBL_ROUND)
    // -------------------------------------------------------------
    // 円形ウォッチ（Chalk / Gabbro）: 中心線寄せレイアウト
    // 左側（TIME）＝右端揃え、右側（中段連動）＝左端揃え
    // 上側＝数値行、下側＝ラベル行
    // -------------------------------------------------------------
    int gap = 6;
#if defined(PBL_PLATFORM_GABBRO)
    gap = 8;
    int val_y = map_h + 4;
    int val_h = 30;
    int lbl_y = map_h + 36;
    int lbl_h = 20;
#else
    // Chalk (180x180)
    int val_y = map_h + 2;
    int val_h = 22;
    int lbl_y = map_h + 24;
    int lbl_h = 16;
#endif

    int left_col_w = (wt / 2) - gap;
    int right_col_x = (wt / 2) + gap;
    int right_col_w = left_col_w;

    // 上側：数値行（中心線に寄せて左右に配置）
    graphics_draw_text(ctx, time_str, data_font, GRect(0, val_y, left_col_w, val_h), 
        GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    graphics_draw_text(ctx, mid_val_unit, data_font, GRect(right_col_x, val_y, right_col_w, val_h), 
        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    // 下側：ラベル行（中心線に寄せて左右に配置）
    graphics_draw_text(ctx, "TIME", label_font, GRect(0, lbl_y, left_col_w, lbl_h), 
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    graphics_draw_text(ctx, mid_name, label_font, GRect(right_col_x, lbl_y, right_col_w, lbl_h), 
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

#else
    // -------------------------------------------------------------
    // 矩形ウォッチ（Basalt / Diorite / Aplite / Flint / Emery）
    // 上段＝ラベル行、下段＝数値行
    // -------------------------------------------------------------
    int pad_x = 6;
    int label_y = map_h + 2;
    int value_y = map_h + 15;
    int label_h = 13;
    int value_h = 24;

#if defined(PBL_PLATFORM_EMERY)
    pad_x = 10;
    label_y = map_h + 3;
    value_y = map_h + 18;
    label_h = 15;
    value_h = 30;
#endif

    int content_w = wt - (pad_x * 2);
    int col_w = content_w / 2;

    // 上段ラベル描画（白文字・Gothic）
    graphics_draw_text(ctx, "TIME", label_font, GRect(pad_x, label_y, col_w, label_h), 
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, mid_name, label_font, GRect(pad_x + col_w, label_y, col_w, label_h), 
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

    // 下段数値描画（左右完全統一フォント）
    graphics_draw_text(ctx, time_str, data_font, GRect(pad_x, value_y, col_w, value_h), 
        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, mid_val_unit, data_font, GRect(pad_x + col_w, value_y, col_w, value_h), 
        GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
#endif

    // 3. 上部現在時刻HUD（小さく中央配置）
    int hud_w = 56;
    int hud_h = 16;
#if defined(PBL_PLATFORM_GABBRO)
    hud_w = 70;
    hud_h = 20;
#endif
    int hud_x = (wt - hud_w) / 2;
    int hud_y = 3;
#if defined(PBL_ROUND)
    hud_y = 8;
#endif
    GRect hud_rect = GRect(hud_x, hud_y, hud_w, hud_h);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, hud_rect, 4, GCornersAll);
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_draw_rect(ctx, hud_rect);

    const char *clk = s_clock_ptr ? s_clock_ptr : "";
    graphics_context_set_text_color(ctx, GColorWhite);
    GFont hud_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
#if defined(PBL_PLATFORM_GABBRO)
    hud_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
#endif
    graphics_draw_text(ctx, clk, hud_font, 
        GRect(hud_x, hud_y - 2, hud_w, hud_h), 
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

void ui_map_create(Window *window, GColor main_bg, GColor main_fg) {
    if (s_map_layer != NULL) return;
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    s_is_map_active = true;
    // 背景受信済みのデータ(s_has_received_data)があれば破棄せず即座に描画

    Layer *wl = window_get_root_layer(window);
    GRect b = layer_get_bounds(wl);
    s_map_layer = layer_create(b);
    layer_set_update_proc(s_map_layer, map_update_proc);
    layer_add_child(wl, s_map_layer);
}

void ui_map_destroy(void) {
    s_is_map_active = false;
    s_has_received_data = false;
    s_transfer_started_from_zero = false;
    s_map_rle_len = 0;
    s_received_chunks_mask = 0;
    s_expected_total_chunks = 0;
    if (s_map_layer) {
        layer_destroy(s_map_layer);
        s_map_layer = NULL;
    }
}

bool ui_map_is_active(void) {
    return s_is_map_active;
}

void ui_map_update_data(const uint8_t *data, int length, int chunk_idx, int total_chunks) {
    if (!data || length <= 0 || total_chunks <= 0 || total_chunks > 32) {
        return;
    }

    // 必ず chunk_idx == 0 で新しいマップフレームの受信をクリーンに開始する。
    // これにより、前回と同じチャンク数のマップが送られてきた場合でも旧チャンクと新チャンクが絶対に混ざらない。
    if (chunk_idx == 0) {
        s_map_rle_len = 0;
        s_has_received_data = false;
        s_received_chunks_mask = 0;
        s_expected_total_chunks = total_chunks;
        s_transfer_started_from_zero = true;
    } else if (!s_transfer_started_from_zero || s_expected_total_chunks != total_chunks) {
        // チャンク0をまだ受け取っていない状態での途中チャンクや世代違いのチャンクは破棄
        s_has_received_data = false;
        s_received_chunks_mask = 0;
        s_transfer_started_from_zero = false;
        return;
    }

    // RLEストリームおよびチャンク分割サイズは常に偶数バイト（2バイトペア単位）。
    // 万が一通信異常で奇数バイトのチャンクが届いた場合は切り捨てて[count, color]の反転（カラー砂嵐）を防ぐ。
    int aligned_length = (length / 2) * 2;
    if (aligned_length <= 0) {
        return;
    }

    if (chunk_idx >= 0 && chunk_idx < total_chunks && chunk_idx < 32) {
        int offset = chunk_idx * MAP_CHUNK_SIZE;
        if (offset + aligned_length <= MAX_MAP_RLE_SIZE) {
            memcpy(s_map_rle_data + offset, data, aligned_length);
            s_received_chunks_mask |= (1UL << chunk_idx);
            if (offset + aligned_length > s_map_rle_len) {
                s_map_rle_len = offset + aligned_length;
            }
        }
    }

    // 全チャンク(0 .. total_chunks-1)が1つも欠けずに揃った場合のみ描画フラグを立てて更新
    uint32_t full_mask = (total_chunks == 32) ? 0xFFFFFFFFUL : ((1UL << total_chunks) - 1UL);
    if (s_transfer_started_from_zero && s_received_chunks_mask == full_mask) {
        s_has_received_data = true;
        s_transfer_started_from_zero = false; // 次の転送は必ず再度 chunk_idx == 0 から始まることを強制
        if (s_map_layer) {
            layer_mark_dirty(s_map_layer);
        }
    }
}

void ui_map_mark_dirty(void) {
    if (s_map_layer) {
        layer_mark_dirty(s_map_layer);
    }
}

void ui_map_update_colors(GColor main_bg, GColor main_fg) {
    s_current_bg = main_bg;
    s_current_fg = main_fg;
    if (s_map_layer) {
        layer_mark_dirty(s_map_layer);
    }
}

#endif
