#include "ui_marquee.h"

static Window *s_window = NULL;
static ActionBarLayer *s_action_bar = NULL;
static Layer *s_graph_layer = NULL;

static TextLayer *s_msg_layer = NULL;
static Layer *s_msg_container_layer = NULL;
static PropertyAnimation *s_marquee_anim = NULL;
static AppTimer *s_marquee_timer = NULL;
static bool s_is_custom_marquee = false;
static GColor s_current_fg, s_current_bg;
static uint8_t s_current_app_state = 0;

static void marquee_timer_callback(void *context);
static void create_marquee_layers(void);

static void msg_container_update_proc(Layer *layer, GContext *ctx) {
    graphics_context_set_fill_color(ctx, s_current_bg);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void ui_marquee_init(Window *window, ActionBarLayer *action_bar, Layer *graph_layer) {
    s_window = window;
    s_action_bar = action_bar;
    s_graph_layer = graph_layer;
}

void ui_marquee_set_action_bar(ActionBarLayer *action_bar) {
    s_action_bar = action_bar;
}

void ui_marquee_set_graph_layer(Layer *graph_layer) {
    s_graph_layer = graph_layer;
}

void ui_marquee_stop(void) {
    if (s_marquee_anim) {
        animation_unschedule(property_animation_get_animation(s_marquee_anim));
        s_marquee_anim = NULL;
    }
    if (s_marquee_timer) {
        app_timer_cancel(s_marquee_timer);
        s_marquee_timer = NULL;
    }
}

void ui_marquee_destroy(void) {
    ui_marquee_stop();
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

bool ui_marquee_is_active(void) { return (s_msg_container_layer != NULL); }
bool ui_marquee_is_custom(void) { return s_is_custom_marquee; }

static void create_marquee_layers(void) {
    if (s_msg_container_layer != NULL || !s_window) return;
    
    Layer *wl = window_get_root_layer(s_window);
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
    
    if (finished && (s_current_app_state == 5 || s_current_app_state == 6)) {
        ui_marquee_destroy();
        if (s_marquee_timer) app_timer_cancel(s_marquee_timer);
        s_marquee_timer = app_timer_register(5000, marquee_timer_callback, NULL);
    } else if (finished && (s_current_app_state < 3)) {
        ui_marquee_trigger(s_current_app_state, s_current_fg, s_current_bg);
    } else if (finished) {
        ui_marquee_destroy();
    }
}

static void marquee_timer_callback(void *context) {
    s_marquee_timer = NULL;
    ui_marquee_trigger(s_current_app_state, s_current_fg, s_current_bg);
}

static void custom_msg_timer_callback(void *context) {
    s_marquee_timer = NULL;
    s_is_custom_marquee = false;
    if (s_current_app_state == 5 || s_current_app_state == 6) {
        ui_marquee_destroy();
        s_marquee_timer = app_timer_register(5000, marquee_timer_callback, NULL);
    } else if (s_current_app_state < 3) {
        ui_marquee_trigger(s_current_app_state, s_current_fg, s_current_bg);
    } else {
        ui_marquee_destroy();
    }
}

void ui_marquee_trigger_custom(const char *msg, GColor fg_color, GColor bg_color, uint8_t app_state) {
    s_current_fg = fg_color;
    s_current_bg = bg_color;
    s_current_app_state = app_state;
    s_is_custom_marquee = true;
    
    create_marquee_layers();
    ui_marquee_stop();
    
    static char custom_msg_buf[64];
    snprintf(custom_msg_buf, sizeof(custom_msg_buf), "%s", msg);
    
    text_layer_set_text(s_msg_layer, custom_msg_buf);
    text_layer_set_text_color(s_msg_layer, s_current_fg);
    text_layer_set_text_alignment(s_msg_layer, GTextAlignmentCenter);
    
    Layer *wl = window_get_root_layer(s_window);
#if defined(PBL_ROUND)
    int w = layer_get_bounds(wl).size.w;
#else
    int w = layer_get_bounds(wl).size.w - ACTION_BAR_WIDTH;
#endif
    
    layer_set_frame(text_layer_get_layer(s_msg_layer), GRect(0, -2, w, 24));
    s_marquee_timer = app_timer_register(3000, custom_msg_timer_callback, NULL);
}

void ui_marquee_trigger(uint8_t app_state, GColor fg_color, GColor bg_color) {
    s_current_fg = fg_color;
    s_current_bg = bg_color;
    s_current_app_state = app_state;

    if (s_is_custom_marquee) return;

    if (s_current_app_state == 3 || s_current_app_state == 4) {
        ui_marquee_destroy();
        return;
    }

    create_marquee_layers();
    ui_marquee_stop();
    
    static char msg_buf[64];
    if (s_current_app_state == 0) {
        snprintf(msg_buf, sizeof(msg_buf), "PRESS [UP] TO START OR SET UP ON PHONE ...");
    } else if (s_current_app_state == 1) {
        snprintf(msg_buf, sizeof(msg_buf), "SEARCHING GPS ...");
    } else if (s_current_app_state == 2) {
        snprintf(msg_buf, sizeof(msg_buf), "READY TO START !");
    } else if (s_current_app_state == 5) {
        snprintf(msg_buf, sizeof(msg_buf), "FINISH? [UP] SAVE [DOWN] DISCARD");
    } else if (s_current_app_state == 6) {
        snprintf(msg_buf, sizeof(msg_buf), "SAVED ! PRESS SELECT TO RESET");
    } else {
        return;
    }
    
    text_layer_set_text(s_msg_layer, msg_buf);
    text_layer_set_text_color(s_msg_layer, s_current_fg);
    text_layer_set_text_alignment(s_msg_layer, GTextAlignmentLeft);
    
    Layer *wl = window_get_root_layer(s_window);
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
