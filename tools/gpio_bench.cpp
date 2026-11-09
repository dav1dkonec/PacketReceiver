// Measures the average cost of GpioToggle::toggle().
// usage: gpio_bench [chip] [line] [count]
#include "gpio_toggle.h"

#include <cstdio>
#include <string>
#include <time.h>

int main(int argc, char* argv[])
{
    const std::string chip = argc > 1 ? argv[1] : "/dev/gpiochip0";
    const unsigned line    = argc > 2 ? std::stoul(argv[2]) : 17;
    const long count       = argc > 3 ? std::stol(argv[3]) : 1'000'000;

    GpioToggle gpio(chip, line);

    timespec t0{}, t1{};
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (long i = 0; i < count; ++i) {
        gpio.toggle();
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    const double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    std::printf("%ld toggles, %.1f ns per toggle\n", count, ns / count);
}
