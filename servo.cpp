#include "pico/stdlib.h"
#include "hardware/pwm.h"

class Servo
{
public:
    Servo(uint gpio, uint16_t min_us = 1000, uint16_t max_us = 2000)
        : gpio_(gpio), min_pulse_(min_us), max_pulse_(max_us)
    {
        gpio_set_function(gpio_, GPIO_FUNC_PWM);
        slice_num_ = pwm_gpio_to_slice_num(gpio_);

        // Configure for ~50Hz (20ms period)
        pwm_set_clkdiv(slice_num_, 64);     // divide 125 MHz by 64
        pwm_set_wrap(slice_num_, WRAP_VAL); // gives ~50 Hz period
        pwm_set_enabled(slice_num_, true);
    }

    void set_angle(uint16_t angle)
    {
        if (angle > 180)
            angle = 180;

        // Map 0–180° → min_pulse_–max_pulse_ (in microseconds)
        uint32_t pulse = min_pulse_ + ((uint32_t)(max_pulse_ - min_pulse_) * angle) / 180;

        // Convert microseconds → level (duty cycle)
        uint32_t level = (pulse * WRAP_VAL) / PERIOD_US;

        pwm_set_gpio_level(gpio_, (uint16_t)level);
    }

private:
    static constexpr uint32_t PERIOD_US = 20000; // 20 ms period
    static constexpr uint32_t WRAP_VAL = 39062;  // 125 MHz / 64 / 50 Hz

    uint gpio_;
    uint slice_num_;
    uint16_t min_pulse_;
    uint16_t max_pulse_;
};