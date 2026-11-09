//
// Created by David Konečný on 09.11.2026.
//

#ifndef LATENCY_RX_GPIO_TOGGLE_H
#define LATENCY_RX_GPIO_TOGGLE_H

#include <gpiod.hpp>
#include <string>

/// One GPIO output line, held for the lifetime of the object (RAII).
/// toggle() is the only call on the hot path: one ioctl, no allocation.
class GpioToggle {
public:
    /// Requests @p line on @p chip as an output, initially low.
    /// @throws std::system_error if the chip can't be opened or the line is busy.
    GpioToggle(const std::string& chip, unsigned line);

    GpioToggle(const GpioToggle&) = delete;             // the line has one owner
    GpioToggle& operator=(const GpioToggle&) = delete;

    // Flips the output: every received packet produces exactly one edge.
    void toggle()
    {
        state_ = !state_;
        request_.set_value(line_, state_ ? gpiod::line::value::ACTIVE
                                         : gpiod::line::value::INACTIVE);
    }

    bool state() const { return state_; }

private:
    gpiod::line::offset line_;      // must come before request_ (init order)
    gpiod::line_request request_;
    bool state_ = false;
};

#endif //LATENCY_RX_GPIO_TOGGLE_H
