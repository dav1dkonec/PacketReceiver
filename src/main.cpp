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
#include "packet_socket.h"
#include "realtime.h"
#include "rx_loop.h"
#include "sample_log.h"
#include "spin_udp_socket.h"
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

        unsigned napi_id = 0;  // read after the loop, from the last packet

        switch (cfg.mode) {
        case RxMode::Udp: {
            UdpSocket sock(cfg.port);
            run_loop(sock, gpio, cfg, stats, log);
            napi_id = sock.napi_id();
            break;
        }
        case RxMode::BusyPoll: {
            UdpSocket sock(cfg.port, cfg.busy_poll_us);
            std::cout << "busy poll " << cfg.busy_poll_us
                      << " us (no effect if packets are further apart)\n";
            run_loop(sock, gpio, cfg, stats, log);
            napi_id = sock.napi_id();
            break;
        }
        case RxMode::BusySpin: {
            SpinUdpSocket sock(cfg.port);
            if (cfg.realtime && rt_throttling_enabled()) {
                std::cout << "warning: RT throttling on, the spinning task is paused up to 50 ms/s "
                             "when other tasks run on this CPU "
                             "(echo -1 > /proc/sys/kernel/sched_rt_runtime_us)\n";
            }
            run_loop(sock, gpio, cfg, stats, log);
            napi_id = sock.napi_id();
            break;
        }
        case RxMode::Packet: {
            PacketSocket sock(cfg.interface, cfg.port);
            run_loop(sock, gpio, cfg, stats, log);
            break;
        }
        case RxMode::Xdp:
            throw std::runtime_error("mode xdp: kernel has no AF_XDP support (CONFIG_XDP_SOCKETS)");
        }

        // a second Ctrl+C now kills at once, possibly truncating the CSV
        std::cout << "\nstopped\n";
        stats.print(std::cout);
        // 0 = loopback or no NAPI: busy polling had nothing to poll
        if (cfg.mode != RxMode::Packet) {
            std::cout << "napi id " << napi_id << '\n';
        }
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
