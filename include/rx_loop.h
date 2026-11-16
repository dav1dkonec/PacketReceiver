#ifndef LATENCY_RX_RX_LOOP_H
#define LATENCY_RX_RX_LOOP_H

#include <sys/types.h>  // ssize_t

#include <cerrno>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <span>
#include <system_error>

#include "config.h"
#include "gpio_toggle.h"
#include "realtime.h"
#include "sample_log.h"
#include "stats.h"
#include "stop_signal.h"

inline uint64_t realtime_ns()
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);  // same clock as SO_TIMESTAMPNS
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
           + static_cast<uint64_t>(ts.tv_nsec);
}

/// What run_loop needs from a receive mode.
template <class R>
concept Receiver = requires(R& r, void* buf, const unsigned char* data, std::size_t len) {
    // blocks or spins; -1 with EINTR (or g_stop set) on a stop signal
    { r.receive(buf, len) } -> std::same_as<ssize_t>;
    { r.last_rx_timestamp_ns() } -> std::same_as<uint64_t>;
    // test packet inside the received bytes (empty if malformed)
    { r.payload(data, len) } -> std::same_as<std::span<const unsigned char>>;
};

/// Receive loop shared by all modes; runs until SIGINT/SIGTERM.
/// A template, not a virtual interface: each mode gets its own copy with
/// rx.receive() inlined, so no indirect call sits before the toggle.
template <Receiver Rx>
void run_loop(Rx& rx, GpioToggle& gpio, const Config& cfg, Stats& stats, SampleLog& log)
{
    // > MTU (1500 B): oversized datagrams are reported as invalid, not cut to 16 B
    unsigned char buf[2048];

    // last, after all allocations: mlockall locks them, setup runs non-RT
    if (cfg.realtime) {
        setup_realtime(cfg.cpu, cfg.priority);
        std::cout << "RT: cpu " << cfg.cpu << ", SCHED_FIFO " << cfg.priority
                  << ", memory locked\n";
    }

    std::cout << "receiving (Ctrl+C to stop)\n";
    // a signal just before recv() starts is noticed only after the next packet
    while (!g_stop) {
        const ssize_t n = rx.receive(buf, sizeof buf);
        gpio.toggle();  // GOLDEN RULE: nothing between recv() and toggle()

        if (n < 0) {
            // EINTR from the stop signal → the while condition ends the loop.
            // errno is not read because toggle() ran after recv().
            // This made one extra edge, discarded in the evaluation.
            if (g_stop) {
                continue;
            }
            throw std::system_error(errno, std::generic_category(), "recv");
        }

        // timestamp first, as close to the GPIO edge as possible
        const uint64_t user_ns = realtime_ns();
        const uint64_t kernel_ns = rx.last_rx_timestamp_ns();
        const auto p = rx.payload(buf, static_cast<std::size_t>(n));
        const int64_t seq = stats.on_packet(p.data(), p.size());
        stats.on_latency(kernel_ns, user_ns);
        log.add({seq, kernel_ns, user_ns});

        // a packet arriving while printing waits longer (large -r avoids it)
        if (stats.received % cfg.report_every == 0) {
            stats.print(std::cout);
        }
    }
}

#endif // LATENCY_RX_RX_LOOP_H
