//
// Created by David Konečný on 27.10.2026.
//

#ifndef RECIEVER_CONFIG_H
#define RECIEVER_CONFIG_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// ---------------------------------------------------------------------------
// Receiver configuration
// ---------------------------------------------------------------------------

/// Mode for receiving packets (selected by --mode).
enum class RxMode {
    Udp,       ///< blocking recv() on a UDP socket, interrupt driven
    BusyPoll,  ///< blocking recv() with kernel busy polling (SO_BUSY_POLL)
    BusySpin,  ///< own polling loop: recv(..., MSG_DONTWAIT)
    Packet,    ///< AF_PACKET raw frames, bypasses the kernel IP/UDP stack
    Xdp,       ///< AF_XDP (optional, needs native XDP support in the driver)
};

/// Mode name as used on the command line, e.g. "busy-poll".
std::string_view to_string(RxMode mode);

struct Config {
    // network
    uint16_t port = 5000; // UDP port for packet testing
    std::string interface = "eth0"; // used by packet / xdp modes
    RxMode mode = RxMode::Udp; // receive method
    int busy_poll_us = 50; // SO_BUSY_POLL budget in us (busy-poll mode)

    // gpio
    std::string gpio_chip = "/dev/gpiochip0"; // verify with: gpiodetect
    unsigned gpio_line = 17; // physical pin 11

    // real-time settings for cpu
    bool realtime = true; // --no-rt turns off RT settings
    int cpu = 3; // core
    int priority = 80; // SCHED_FIFO priority (1-99)

    // statistics and results
    uint32_t report_every = 1000; // report periodically every N packets
    std::string output_csv; // empty = do not save samples
    std::size_t max_samples = 1'000'000; // capacity of the preallocated sample buffer
};

/**
 * @brief Parses command-line arguments into a Config.
 *
 * Every option is optional; values not given on the command line keep
 * the defaults from Config.
 *
 * Supported options:
 *   -p, --port <n>          UDP port (1-65535)
 *   -i, --interface <name>  network interface, e.g. eth0
 *   -m, --mode <mode>       udp | busy-poll | busy-spin | packet | xdp
 *       --busy-poll-us <n>  SO_BUSY_POLL budget in microseconds
 *   -c, --gpio-chip <path>  GPIO chip device, e.g. /dev/gpiochip0
 *   -l, --line <n>          GPIO line offset
 *       --no-rt             disable real-time setup (affinity, SCHED_FIFO, mlockall)
 *       --cpu <n>           CPU core to pin the process to
 *       --fifo-prio <n>     SCHED_FIFO priority (1-99)
 *   -r, --report-every <n>  print statistics every n packets
 *   -o, --output <file>     save samples to CSV at the end of the run
 *       --samples <n>       capacity of the sample buffer
 *   -h, --help              print usage and exit
 *
 * @param argc  Argument count, as passed to main().
 * @param argv  Argument vector, as passed to main().
 * @return      Configuration with defaults overridden by the given options.
 *
 * @throws std::invalid_argument  Unknown option, missing value, or a value
 *                                that is not a number / out of range.
 *
 * @note `--help` prints usage to stdout and terminates the program
 *       with exit code 0; the function does not return in that case.
 */
Config parse_args(int argc, char* argv[]);

/**
 * @brief Prints the list of options with their default values to stdout.
 *
 * @param prog  Program name shown in the usage line, typically argv[0].
 */
void print_usage(const char* prog);

#endif //RECIEVER_CONFIG_H
