/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_num_word

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/keys.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct num_word_continue_item {
    uint16_t page;
    uint32_t id;
    uint8_t implicit_modifiers;
};

struct behavior_num_word_config {
    zmk_mod_flags_t mods;
    int8_t layers;
    bool ignore_alphas;
    bool ignore_numbers;
    bool ignore_modifiers;
    uint8_t index;
    uint8_t continuations_count;
    struct num_word_continue_item continuations[];
};

struct behavior_num_word_data {
    bool active;
};

static void activate_num_word(const struct device *dev) {
    struct behavior_num_word_data *data = dev->data;

    const struct behavior_num_word_config *config = dev->config;

    if (config->layers > -1) {
        zmk_keymap_layer_activate(config->layers);
    }
    data->active = true;
}

static void deactivate_num_word(const struct device *dev) {
    struct behavior_num_word_data *data = dev->data;

    const struct behavior_num_word_config *config = dev->config;

    if (config->layers > -1) {
        zmk_keymap_layer_deactivate(config->layers);
    }
    data->active = false;
}

static int on_num_word_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_num_word_data *data = dev->data;

    if (data->active) {
        deactivate_num_word(dev);
    } else {
        activate_num_word(dev);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_num_word_binding_released(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_num_word_driver_api = {
    .binding_pressed = on_num_word_binding_pressed,
    .binding_released = on_num_word_binding_released,
};

static int num_word_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_num_word, num_word_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_num_word, zmk_keycode_state_changed);

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static bool num_word_is_caps_includelist(const struct behavior_num_word_config *config,
                                          uint16_t usage_page, uint8_t usage_id,
                                          uint8_t implicit_modifiers) {
    for (int i = 0; i < config->continuations_count; i++) {
        const struct num_word_continue_item *continuation = &config->continuations[i];
        LOG_DBG("Comparing with 0x%02X - 0x%02X (with implicit mods: 0x%02X)", continuation->page,
                continuation->id, continuation->implicit_modifiers);

        if (continuation->page == usage_page && continuation->id == usage_id &&
            (continuation->implicit_modifiers &
             (implicit_modifiers | zmk_hid_get_explicit_mods())) ==
                continuation->implicit_modifiers) {
            LOG_DBG("Continuing capsword, found included usage: 0x%02X - 0x%02X", usage_page,
                    usage_id);
            return true;
        }
    }

    return false;
}

static bool num_word_is_alpha(uint8_t usage_id) {
    return (usage_id >= HID_USAGE_KEY_KEYBOARD_A && usage_id <= HID_USAGE_KEY_KEYBOARD_Z);
}

static bool num_word_is_numeric(uint8_t usage_id) {
    return (usage_id >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION &&
            usage_id <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS);
}

static void num_word_enhance_usage(const struct behavior_num_word_config *config,
                                    struct zmk_keycode_state_changed *ev) {
    if (ev->usage_page != HID_USAGE_KEY || !num_word_is_alpha(ev->keycode)) {
        return;
    }

    LOG_DBG("Enhancing usage 0x%02X with modifiers: 0x%02X", ev->keycode, config->mods);
    ev->implicit_modifiers |= config->mods;
}

static int num_word_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }

        struct behavior_num_word_data *data = dev->data;
        if (!data->active) {
            continue;
        }

        const struct behavior_num_word_config *config = dev->config;

        num_word_enhance_usage(config, ev);

        if ((!num_word_is_alpha(ev->keycode) || !config->ignore_alphas) &&
            (!num_word_is_numeric(ev->keycode) || !config->ignore_numbers) &&
            (!is_mod(ev->usage_page, ev->keycode) || !config->ignore_modifiers) &&
            !num_word_is_caps_includelist(config, ev->usage_page, ev->keycode,
                                           ev->implicit_modifiers)) {
            LOG_DBG("Deactivating num_word for 0x%02X - 0x%02X", ev->usage_page, ev->keycode);
            deactivate_num_word(dev);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_num_word_init(const struct device *dev) {
    const struct behavior_num_word_config *config = dev->config;
    devs[config->index] = dev;
    return 0;
}

#define num_word_LABEL(i, _n) DT_INST_LABEL(i)

#define PARSE_BREAK(i)                                                                             \
    {                                                                                              \
        .page = ZMK_HID_USAGE_PAGE(i), .id = ZMK_HID_USAGE_ID(i),                                  \
        .implicit_modifiers = SELECT_MODS(i)                                                       \
    }

#define BREAK_ITEM(i, n) PARSE_BREAK(DT_INST_PROP_BY_IDX(n, continue_list, i))

#define KP_INST(n)                                                                                 \
    static struct behavior_num_word_data behavior_num_word_data_##n = {.active = false};         \
    static struct behavior_num_word_config behavior_num_word_config_##n = {                      \
        .index = n,                                                                                \
        .mods = DT_INST_PROP_OR(n, mods, 0),                                                       \
        .layers = DT_INST_PROP_OR(n, layers, -1),                                                  \
        .ignore_alphas = DT_INST_PROP(n, ignore_alphas),                                           \
        .ignore_numbers = DT_INST_PROP(n, ignore_numbers),                                         \
        .ignore_modifiers = DT_INST_PROP(n, ignore_modifiers),                                     \
        .continuations = {LISTIFY(DT_INST_PROP_LEN(n, continue_list), BREAK_ITEM, (, ), n)},       \
        .continuations_count = DT_INST_PROP_LEN(n, continue_list),                                 \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_num_word_init, NULL, &behavior_num_word_data_##n,        \
                            &behavior_num_word_config_##n, APPLICATION,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_num_word_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif
