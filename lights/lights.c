/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "lights.qcom"

#include <errno.h>
#include <fcntl.h>
#include <hardware/lights.h>
#include <log/log.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BACKLIGHT_BRIGHTNESS "/sys/class/leds/lcd-backlight/brightness"
#define BACKLIGHT_MAX_BRIGHTNESS "/sys/class/leds/lcd-backlight/max_brightness"

static int read_max_brightness(void) {
    char buffer[16] = {};
    int fd = open(BACKLIGHT_MAX_BRIGHTNESS, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return -errno;

    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytes <= 0)
        return bytes < 0 ? -errno : -EIO;

    return (int)strtol(buffer, NULL, 10);
}

static int write_brightness(int brightness) {
    char buffer[16];
    int fd = open(BACKLIGHT_BRIGHTNESS, O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        return -errno;

    int length = snprintf(buffer, sizeof(buffer), "%d\n", brightness);
    ssize_t bytes = write(fd, buffer, (size_t)length);
    close(fd);
    return bytes == length ? 0 : (bytes < 0 ? -errno : -EIO);
}

static int rgb_to_brightness(const struct light_state_t *state) {
    int color = state->color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff)) +
            (150 * ((color >> 8) & 0xff)) +
            (29 * (color & 0xff))) >> 8;
}

static int set_backlight(struct light_device_t *device,
                         const struct light_state_t *state) {
    (void)device;
    int max_brightness = read_max_brightness();
    if (max_brightness <= 0) {
        ALOGE("Unable to read %s: %d", BACKLIGHT_MAX_BRIGHTNESS,
              max_brightness);
        return max_brightness ? max_brightness : -EIO;
    }

    int brightness = rgb_to_brightness(state);
    return write_brightness((brightness * max_brightness + 127) / 255);
}

static int close_lights(struct hw_device_t *device) {
    free(device);
    return 0;
}

static int open_lights(const struct hw_module_t *module, const char *name,
                       struct hw_device_t **device) {
    if (strcmp(name, LIGHT_ID_BACKLIGHT) != 0)
        return -EINVAL;

    struct light_device_t *light = calloc(1, sizeof(*light));
    if (light == NULL)
        return -ENOMEM;

    light->common.tag = HARDWARE_DEVICE_TAG;
    light->common.version = LIGHTS_DEVICE_API_VERSION_2_0;
    light->common.module = (struct hw_module_t *)module;
    light->common.close = close_lights;
    light->set_light = set_backlight;
    *device = &light->common;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open = open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "GM9PRO backlight HAL",
    .author = "LineageOS",
    .methods = &lights_module_methods,
};
