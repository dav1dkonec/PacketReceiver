//
// Created by David Konečný on 09.11.2026.
//

#include "gpio_toggle.h"

namespace {

gpiod::line_request request_output(const std::string& chip, gpiod::line::offset line)
{
  return gpiod::chip(chip)
      .prepare_request()
      .set_consumer("latency_rx")      // shown by gpioinfo
      .add_line_settings(line, gpiod::line_settings()
                                   .set_direction(gpiod::line::direction::OUTPUT)
                                   .set_output_value(gpiod::line::value::INACTIVE))
      .do_request();
}

} // namespace

GpioToggle::GpioToggle(const std::string& chip, unsigned line)
  : line_(line), request_(request_output(chip, line_))
{
}
