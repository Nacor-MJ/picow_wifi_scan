/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "SparkFun_TB6612.h"
// TODO: make a header file for this
#include "servo.cpp"
#include "tcp_server.hpp"

// includes the char ssid[] and char pass[]
#include "wifi.h"

Motor motor(26, 27, 28, 1, 5);
Servo servo(5);

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

volatile int motor_drive = 0;
int last_motor_drive = motor_drive - 1;
void loop_motor()
{
    if (last_motor_drive == motor_drive)
    {
        return;
    }

    printf("motor drive: %d\n", motor_drive);
    if (motor_drive == 0)
    {
        motor.brake();
    }
    else
    {
        motor.drive(motor_drive);
    }

    last_motor_drive = motor_drive;
}

volatile int servo_dir = 90;
int last_servo_dir = servo_dir - 1;
void loop_servo()
{
    if (last_servo_dir == servo_dir)
    {
        return;
    }

    printf("servo_dir: %d\n", servo_dir);
    servo.set_angle(servo_dir);

    last_servo_dir = servo_dir;
}

void loop_server()
{
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

    // Optionally pass netif pointer for nicer logging. Here we use netif_list from lwIP.
    extern struct netif *netif_list;
    pico_tcp::TcpServer server(netif_list);

    if (!server.start())
    {
        printf("Server failed to start\n");
        cyw43_arch_deinit();
        return 1;
    }
    printf("server started");

    bool led_on = false;
    bool exit = false;
    const uint64_t loop_time = 100000;
    absolute_time_t next_loop;

    while (!exit || !server.is_complete())
    {
        next_loop = get_absolute_time() + loop_time;

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        led_on = !led_on;

        loop_motor();
        loop_servo();

        cyw43_arch_poll();

        // waits for `loop_time` us
        cyw43_arch_wait_for_work_until(next_loop);
    }

    int status = server.last_status();
    printf("Done. status=%d\n", status);

    motor.brake();
    cyw43_arch_deinit();
    return 0;
}
