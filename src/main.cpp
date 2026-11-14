// latency_rx – test packet receiver for measuring network stack latency.
// Toggles a GPIO when a packet arrives; a Pico measures wire → GPIO edge.
// GOLDEN RULE: nothing runs between recv() and the toggle, it would add to
// the measured latency.

#include "config.h"

#include <cerrno>
#include <csignal>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include "gpio_toggle.h"
#include "sample_log.h"
#include "stats.h"
#include "udp_socket.h"

namespace {

// sig_atomic_t: written atomically; volatile: re-read on every loop check
volatile std::sig_atomic_t g_stop = 0;

void on_signal(int)
{
    // only async-signal-safe work here: the signal may interrupt malloc or cout
    g_stop = 1;
}

void install_stop_handler()
{
    struct sigaction sa{};  // "struct": the function sigaction() has the same name
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    // no SA_RESTART: blocked recv() returns EINTR; SA_RESETHAND: 2nd Ctrl+C kills
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sa, nullptr);   // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr);  // kill, timeout (automation script)
}

uint64_t realtime_ns()
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);  // same clock as SO_TIMESTAMPNS (step 5)
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
           + static_cast<uint64_t>(ts.tv_nsec);
}

} // namespace

int main(int argc, char *argv[]) {
    // no arguments → show the options instead of measuring with all defaults
    if (argc <= 1) {
        print_usage(argv[0]);
        return 0;
    }

    try {
        const Config cfg = parse_args(argc, argv);

        std::cout << "latency_rx: mode " << to_string(cfg.mode)
                  << ", UDP port " << cfg.port
                  << ", interface " << cfg.interface << "\n"
                  << "GPIO " << cfg.gpio_chip << " line " << cfg.gpio_line << "\n"
                  << "RT " << (cfg.realtime ? "on" : "off")
                  << " (cpu " << cfg.cpu << ", prio " << cfg.priority << ")\n"
                  << "output " << (cfg.output_csv.empty() ? "-" : cfg.output_csv) << "\n";

        // open (and truncate) now, so a bad path fails before the measurement
        std::ofstream csv;
        if (!cfg.output_csv.empty()) {
            csv.open(cfg.output_csv);
            if (!csv) {
                throw std::runtime_error("cannot open '" + cfg.output_csv + "' for writing");
            }
        }

        // TODO: setup_realtime (step 6)
        install_stop_handler();

        GpioToggle gpio(cfg.gpio_chip, cfg.gpio_line);
        UdpSocket sock(cfg.port);
        Stats stats;
        // without --output nothing is saved, so nothing is allocated
        SampleLog log(cfg.output_csv.empty() ? 0 : cfg.max_samples);
        // > MTU: oversized datagrams are reported as invalid, not cut to 16 B
        unsigned char buf[UdpSocket::kMaxDatagram];

        std::cout << "listening on UDP port " << cfg.port << " (Ctrl+C to stop)\n";
        // a signal just before recv() starts is noticed only after the next packet
        while (!g_stop) {
            const ssize_t n = sock.receive(buf, sizeof buf);
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
            const int64_t seq = stats.on_packet(buf, static_cast<std::size_t>(n));
            log.add({seq, user_ns});

            // a packet arriving while printing waits longer (large -r avoids it)
            if (stats.received % cfg.report_every == 0) {
                stats.print(std::cout);
            }
        }

        // a second Ctrl+C now kills at once, possibly truncating the CSV
        std::cout << "\nstopped\n";
        stats.print(std::cout);
        if (!cfg.output_csv.empty()) {
            log.write_csv(csv);
            std::cout << "wrote " << log.size() << " samples to " << cfg.output_csv << '\n';
            if (log.dropped() != 0) {
                std::cout << "warning: sample buffer full, " << log.dropped()
                          << " samples not saved (increase --samples)\n";
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"
                  << "Try '" << argv[0] << " --help'.\n";
        return 1;
    }
}
