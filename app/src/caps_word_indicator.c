/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* White at same brightness as layer indicators */
#define CAPS_WORD_HUE 0
#define CAPS_WORD_SAT 0
#define CAPS_WORD_BRT CONFIG_ZMK_RGB_UNDERGLOW_BRT_START

/* Restore default (green) when caps_word deactivates */
#define DEFAULT_HUE CONFIG_ZMK_RGB_UNDERGLOW_HUE_START
#define DEFAULT_SAT CONFIG_ZMK_RGB_UNDERGLOW_SAT_START
#define DEFAULT_BRT CONFIG_ZMK_RGB_UNDERGLOW_BRT_START

static int caps_word_indicator_listener(const zmk_event_t *eh) {
    struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    struct zmk_led_hsb color;
    if (ev->state) {
        LOG_DBG("Caps word activated, setting indicator");
        color = (struct zmk_led_hsb){.h = CAPS_WORD_HUE, .s = CAPS_WORD_SAT, .b = CAPS_WORD_BRT};
    } else {
        LOG_DBG("Caps word deactivated, restoring default");
        color = (struct zmk_led_hsb){.h = DEFAULT_HUE, .s = DEFAULT_SAT, .b = DEFAULT_BRT};
    }

    zmk_rgb_underglow_set_hsb(color);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(caps_word_indicator, caps_word_indicator_listener);
ZMK_SUBSCRIPTION(caps_word_indicator, zmk_caps_word_state_changed);
