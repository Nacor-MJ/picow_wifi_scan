/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "include/SparkFun_TB6612FNG/SparkFun_TB6612.h"

// includes the char ssid[] and char pass[]
#include "wifi.h"

Motor motor(3, 2, 4, 1, 5);

// is responsible for connecting to the wifi
int connect_to_wifi(int retries)
{
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }
    printf("initialized\n");

    cyw43_arch_enable_sta_mode();

    while (retries-- > 0)
    {
        switch (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 3333))
        {
        case PICO_ERROR_BADAUTH:
            printf("failed to connect wrong password, retrying...\n");
            break;
        case PICO_ERROR_CONNECT_FAILED:
            printf("failed to connect, connection failed, retrying...\n");
            break;
        case PICO_ERROR_TIMEOUT:
            printf("failed to connect, connection timed out, retrying...\n");
            break;
        case 0:
            printf("succesfully connected\n");
            goto connected;
        }
    }
    // NOTE: I know this is very stupid but I've always dreamt of using a goto
    // if we fail too many times we fall through here
    return 1;
    // if we succed the goto takes us here
connected:
    return 0;
}

int main()
{
    stdio_init_all();
    printf("\n\n---------------\n");

    if (connect_to_wifi(10))
    {
        printf("failed connection :(");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        return 1;
    }

    bool led_on = false;
    bool exit = false;
    const uint64_t loop_time = 1000000;
    absolute_time_t next_loop;

    while (!exit)
    {
        next_loop = get_absolute_time() + loop_time;

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        led_on = !led_on;

        cyw43_arch_poll();

        // waits for up to 100 ms
        sleep_until(next_loop);
    }

    cyw43_arch_deinit();
    return 0;
}
