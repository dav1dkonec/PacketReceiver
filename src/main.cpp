// latency_rx – test packet receiver for measuring network stack latency.
// Toggles a GPIO when a packet arrives; a Pico measures wire → GPIO edge.
// GOLDEN RULE: nothing runs between recv() and the toggle, it would add to
// the measured latency.

#include "config.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "gpio_toggle.h"
#include "rx_loop.h"
#include "sample_log.h"
#include "stats.h"
#include "stop_signal.h"
#include "udp_socket.h"

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

        install_stop_handler();

        GpioToggle gpio(cfg.gpio_chip, cfg.gpio_line);
        Stats stats;
        // without --output nothing is saved, so nothing is allocated
        SampleLog log(cfg.output_csv.empty() ? 0 : cfg.max_samples);

        switch (cfg.mode) {
        case RxMode::Udp: {
            UdpSocket sock(cfg.port);
            run_loop(sock, gpio, cfg, stats, log);
            break;
        }
        case RxMode::BusyPoll:
        case RxMode::BusySpin:
        case RxMode::Packet:
            throw std::runtime_error("mode " + std::string(to_string(cfg.mode)) + " not implemented yet");
        case RxMode::Xdp:
            throw std::runtime_error("mode xdp: kernel has no AF_XDP support (CONFIG_XDP_SOCKETS)");
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
