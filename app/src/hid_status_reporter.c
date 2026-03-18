/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#if __has_include(<zmk/events/caps_word_state_changed.h>)
#include <zmk/events/caps_word_state_changed.h>
#define HAS_CAPS_WORD 1
#else
#define HAS_CAPS_WORD 0
#endif

#include <raw_hid/events.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * Status report format (4 bytes):
 *   byte 0: report ID (0x01 = status update)
 *   byte 1-2: layer state (16-bit bitset, little-endian)
 *   byte 3: flags (bit 0 = caps_word active)
 */

#define REPORT_ID_STATUS 0x01

static uint8_t report_buf[CONFIG_RAW_HID_REPORT_SIZE];
static uint32_t last_layer_state = 0;
static bool caps_word_active = false;

static void send_status_report(void) {
    memset(report_buf, 0, sizeof(report_buf));
    report_buf[0] = REPORT_ID_STATUS;
    report_buf[1] = last_layer_state & 0xFF;
    report_buf[2] = (last_layer_state >> 8) & 0xFF;
    report_buf[3] = caps_word_active ? 0x01 : 0x00;

    raise_raw_hid_sent_event(
        (struct raw_hid_sent_event){
            .data = report_buf,
            .length = sizeof(report_buf),
        });
}

static int on_layer_state_changed(const zmk_event_t *eh) {
    struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Reconstruct the full layer state from the event */
    if (ev->state) {
        last_layer_state |= BIT(ev->layer);
    } else {
        last_layer_state &= ~BIT(ev->layer);
    }

    LOG_DBG("HID status: layer state 0x%04x", last_layer_state);
    send_status_report();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(hid_status_reporter_layer, on_layer_state_changed);
ZMK_SUBSCRIPTION(hid_status_reporter_layer, zmk_layer_state_changed);

#if HAS_CAPS_WORD
static int on_caps_word_state_changed(const zmk_event_t *eh) {
    struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    caps_word_active = ev->state;
    LOG_DBG("HID status: caps_word %d", caps_word_active);
    send_status_report();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(hid_status_reporter_caps, on_caps_word_state_changed);
ZMK_SUBSCRIPTION(hid_status_reporter_caps, zmk_caps_word_state_changed);
#endif
