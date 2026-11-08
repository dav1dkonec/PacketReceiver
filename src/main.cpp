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

int main(int argc, char *argv[]) {
    try {
        const Config cfg = parse_args(argc, argv);

        std::cout << "latency_rx: mode " << to_string(cfg.mode)
                  << ", UDP port " << cfg.port
                  << ", interface " << cfg.interface << "\n"
                  << "GPIO " << cfg.gpio_chip << " line " << cfg.gpio_line << "\n"
                  << "RT " << (cfg.realtime ? "on" : "off")
                  << " (cpu " << cfg.cpu << ", prio " << cfg.priority << ")\n"
                  << "output " << (cfg.output_csv.empty() ? "-" : cfg.output_csv) << "\n";

        // TODO: setup_realtime, GpioToggle, socket, receive loop
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"
                  << "Try '" << argv[0] << " --help'.\n";
        return 1;
    }
}
