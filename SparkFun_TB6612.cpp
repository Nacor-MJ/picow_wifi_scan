/******************************************************************************
TB6612.cpp
TB6612FNG H-Bridge Motor Driver Example code
Michelle @ SparkFun Electronics
8/20/16
https://github.com/sparkfun/SparkFun_TB6612FNG_Arduino_Library

Uses 2 motors to show examples of the functions in the library.  This causes
a robot to do a little 'jig'.  Each movement has an equal and opposite movement
so assuming your motors are balanced the bot should end up at the same place it
started.

Resources:
TB6612 SparkFun Library

Development environment specifics:
Developed on Arduino 1.6.4
Developed with ROB-9457
******************************************************************************/

#include "SparkFun_TB6612.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdlib.h>

Motor::Motor(int In1pin, int In2pin, int PWMpin, int offset, int STBYpin)
{
   In1 = In1pin;
   In2 = In2pin;
   PWM = PWMpin;
   Standby = STBYpin;
   Offset = offset;

   gpio_init(In1);
   gpio_set_dir(In1, GPIO_OUT);
   gpio_init(In2);
   gpio_set_dir(In2, GPIO_OUT);
   gpio_init(PWM);
   gpio_set_dir(PWM, GPIO_OUT); // For PWM, will be set up later
   gpio_set_function(PWM, GPIO_FUNC_PWM);
   gpio_init(Standby);
   gpio_set_dir(Standby, GPIO_OUT);

   // Setup PWM for PWM pin
   uint slice_num = pwm_gpio_to_slice_num(PWM);
   pwm_set_wrap(slice_num, 255); // 8-bit resolution
   pwm_set_enabled(slice_num, true);
}

void Motor::drive(int speed)
{
   gpio_put(Standby, 1);
   speed = speed * Offset;
   if (speed >= 0)
      fwd(speed);
   else
      rev(-speed);
}

void Motor::fwd(int speed)
{
   gpio_put(In1, true);
   gpio_put(In2, false);
   uint slice_num = pwm_gpio_to_slice_num(PWM);
   pwm_set_gpio_level(PWM, speed);
}

void Motor::rev(int speed)
{
   gpio_put(In1, 0);
   gpio_put(In2, 1);
   uint slice_num = pwm_gpio_to_slice_num(PWM);
   pwm_set_gpio_level(PWM, speed);
}

void Motor::brake()
{
   gpio_put(In1, 1);
   gpio_put(In2, 1);
   uint slice_num = pwm_gpio_to_slice_num(PWM);
   pwm_set_gpio_level(PWM, 0);
}

void Motor::standby()
{
   gpio_put(Standby, 0);
}

void forward(Motor motor1, Motor motor2, int speed)
{
   motor1.drive(speed);
   motor2.drive(speed);
}
void forward(Motor motor1, Motor motor2)
{
   motor1.drive(DEFAULTSPEED);
   motor2.drive(DEFAULTSPEED);
}

void back(Motor motor1, Motor motor2, int speed)
{
   int temp = abs(speed);
   motor1.drive(-temp);
   motor2.drive(-temp);
}
void back(Motor motor1, Motor motor2)
{
   motor1.drive(-DEFAULTSPEED);
   motor2.drive(-DEFAULTSPEED);
}
void left(Motor left, Motor right, int speed)
{
   int temp = abs(speed) / 2;
   left.drive(-temp);
   right.drive(temp);
}

void right(Motor left, Motor right, int speed)
{
   int temp = abs(speed) / 2;
   left.drive(temp);
   right.drive(-temp);
}
void brake(Motor motor1, Motor motor2)
{
   motor1.brake();
   motor2.brake();
}