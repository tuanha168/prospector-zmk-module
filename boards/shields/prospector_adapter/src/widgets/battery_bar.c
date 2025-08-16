#include "battery_bar.h"

#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/event_manager.h>

#include <fonts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

bool initialized = false;

// Simplified state structures for each event type
struct battery_update_state {
    uint8_t source;
    uint8_t level;
};

struct connection_update_state {
    uint8_t source;
    bool connected;
};

static void set_battery_bar_value(lv_obj_t *widget, struct battery_update_state state) {
    if (initialized) {
        lv_obj_t *info_container = lv_obj_get_child(widget, state.source);
        lv_obj_t *bar = lv_obj_get_child(info_container, 0);
        lv_obj_t *num = lv_obj_get_child(info_container, 1);

        lv_bar_set_value(bar, state.level, LV_ANIM_ON);
        lv_label_set_text_fmt(num, "%d", state.level);

        if (state.level < 20) {
            lv_obj_set_style_bg_color(bar, lv_color_hex(0xD3900F), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xE8AC11), LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x6E4E07), LV_PART_MAIN);
            lv_obj_set_style_text_color(num, lv_color_hex(0xFFB802), 0);
        } else {
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x909090), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x202020), LV_PART_MAIN);
            lv_obj_set_style_text_color(num, lv_color_hex(0xFFFFFF), 0);
        }
    }
}

static void set_battery_bar_connected(lv_obj_t *widget, struct connection_update_state state) {
    if (initialized) {
        lv_obj_t *info_container = lv_obj_get_child(widget, state.source);
        lv_obj_t *bar = lv_obj_get_child(info_container, 0);
        lv_obj_t *num = lv_obj_get_child(info_container, 1);
        lv_obj_t *nc_bar = lv_obj_get_child(info_container, 2);
        lv_obj_t *nc_num = lv_obj_get_child(info_container, 3);

        LOG_DBG("Peripheral %d %s", state.source,
                state.connected ? "connected" : "disconnected");

        if (state.connected) {
            lv_obj_fade_out(nc_bar, 150, 0);
            lv_obj_fade_out(nc_num, 150, 0);
            lv_obj_fade_in(bar, 150, 250);
            lv_obj_fade_in(num, 150, 250);
        } else {
            lv_obj_fade_out(bar, 150, 0);
            lv_obj_fade_out(num, 150, 0);
            lv_obj_fade_in(nc_bar, 150, 250);
            lv_obj_fade_in(nc_num, 150, 250);
        }
    }
}

// Battery event handling
void battery_bar_battery_update_cb(struct battery_update_state state) {
    LOG_DBG("Battery update: source=%d, level=%d", state.source, state.level);

    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_bar_value(widget->obj, state);
    }
}

static struct battery_update_state battery_bar_get_battery_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *bat_ev =
        as_zmk_peripheral_battery_state_changed(eh);

    LOG_DBG("Received battery event: source=%d, level=%d", bat_ev->source, bat_ev->state_of_charge);

    return (struct battery_update_state){
        .source = bat_ev->source,
        .level = bat_ev->state_of_charge,
    };
}

// Connection event handling
void battery_bar_connection_update_cb(struct connection_update_state state) {
    LOG_DBG("Connection update: source=%d, connected=%s", state.source, state.connected ? "true" : "false");

    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_bar_connected(widget->obj, state);
    }
}

static struct connection_update_state battery_bar_get_connection_state(const zmk_event_t *eh) {
    const struct zmk_split_central_status_changed *conn_ev =
        as_zmk_split_central_status_changed(eh);

    LOG_DBG("Received connection event: slot=%d, connected=%s", conn_ev->slot, conn_ev->connected ? "true" : "false");

    return (struct connection_update_state){
        .source = conn_ev->slot,
        .connected = conn_ev->connected,
    };
}

// Separate widget listeners for each event type
ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_bar_battery, struct battery_update_state,
                            battery_bar_battery_update_cb, battery_bar_get_battery_state);
ZMK_SUBSCRIPTION(widget_battery_bar_battery, zmk_peripheral_battery_state_changed);

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_bar_connection, struct connection_update_state,
                            battery_bar_connection_update_cb, battery_bar_get_connection_state);
ZMK_SUBSCRIPTION(widget_battery_bar_connection, zmk_split_central_status_changed);

int zmk_widget_battery_bar_init(struct zmk_widget_battery_bar *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_width(widget->obj, lv_pct(100));
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj,
                          LV_FLEX_ALIGN_SPACE_EVENLY, // Main axis (horizontal)
                          LV_FLEX_ALIGN_CENTER,       // Cross axis (vertical)
                          LV_FLEX_ALIGN_CENTER        // Track alignment
    );
    lv_obj_set_style_pad_column(widget->obj, 12, LV_PART_MAIN);
    // lv_obj_set_style_pad_all(widget->obj, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(widget->obj, 12, LV_PART_MAIN);
    // lv_obj_set_style_pad_top(widget->obj, 30, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(widget->obj, 16, LV_PART_MAIN);

    // lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        lv_obj_t *info_container = lv_obj_create(widget->obj);
        lv_obj_center(info_container);
        lv_obj_set_height(info_container, lv_pct(100));
        lv_obj_set_flex_grow(info_container, 1);

        lv_obj_t *bar = lv_bar_create(info_container);
        lv_obj_set_size(bar, lv_pct(100), 4);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x909090), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_dither_mode(bar, LV_DITHER_ERR_DIFF, LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 1, LV_PART_INDICATOR);
        lv_obj_set_style_anim_time(bar, 250, 0);

        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_opa(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_opa(bar, 255, LV_PART_INDICATOR);

        lv_obj_t *num = lv_label_create(info_container);
        lv_obj_set_style_text_font(num, &FoundryGridnikMedium_20, 0);
        lv_obj_set_style_text_color(num, lv_color_white(), 0);
        lv_obj_set_style_opa(num, 255, 0);
        lv_obj_align(num, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(num, "N/A");

        lv_obj_set_style_opa(num, 0, 0);

        lv_obj_t *nc_bar = lv_obj_create(info_container);
        lv_obj_set_size(nc_bar, lv_pct(100), 4);
        lv_obj_align(nc_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(nc_bar, lv_color_hex(0x9e2121), LV_PART_MAIN);
        lv_obj_set_style_radius(nc_bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(nc_bar, 255, 0);

        lv_obj_t *nc_num = lv_label_create(info_container);
        lv_obj_set_style_text_color(nc_num, lv_color_hex(0xe63030), 0);
        lv_obj_align(nc_num, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(nc_num, LV_SYMBOL_CLOSE);
        lv_obj_set_style_opa(nc_num, 255, 0);
    }

    sys_slist_append(&widgets, &widget->node);

    widget_battery_bar_battery_init();
    widget_battery_bar_connection_init();
    initialized = true;

    return 0;
}

lv_obj_t *zmk_widget_battery_bar_obj(struct zmk_widget_battery_bar *widget) { return widget->obj; }