// latency_rx – přijímač testovacích paketů pro měření latence síťového stacku.

// Princip: program čeká na testovací UDP paket. Jakmile ho dostane,
// OKAMŽITĚ přepne výstup GPIO. Pico pak změří čas mezi příchodem rámce
// na kabel (senzor) a touto hranou na GPIO.

// Zlaté pravidlo: mezi návratem z recv() a přepnutím GPIO nesmí být nic
// dalšího – žádný výpis, žádná kontrola, žádná alokace. Všechno, co tam
// dáš, se započítá do měřené latence.

#include "config.h"

#include <exception>
#include <iostream>
#include <cerrno>
#include <system_error>

#include "gpio_toggle.h"
#include "udp_socket.h"
#include "stats.h"

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

        // TODO: setup_realtime (step 6)
        GpioToggle gpio(cfg.gpio_chip, cfg.gpio_line);
        UdpSocket sock(cfg.port);
        Stats stats;
        unsigned char buf[UdpSocket::kMaxDatagram];

        std::cout << "listening on UDP port " << cfg.port << '\n';
        for (;;) {
            const ssize_t n = sock.receive(buf, sizeof buf);
            gpio.toggle();  // GOLDEN RULE: nothing between recv() and toggle()

            if (n < 0) {
                if (errno == EINTR) {
                    continue; // interrupted by a signal (will be handling SIGINT in future)
                }
                throw std::system_error(errno, std::generic_category(), "recv");
            }
            stats.on_packet(buf, static_cast<size_t>(n));
            if (stats.received % cfg.report_every == 0) {
                stats.print(std::cout);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"
                  << "Try '" << argv[0] << " --help'.\n";
        return 1;
    }
}
