//
// Created by David Konečný on 27.10.2026.
//

#ifndef RECIEVER_CONFIG_H
#define RECIEVER_CONFIG_H

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Receiver configuration
// ---------------------------------------------------------------------------

struct Config {
    // network
    uint16_t port = 5000; // UDP port for packet testing

    // gpio
    std::string gpio_chip = "/dev/gpiochip0"; // verify with: gpiodetect
    unsigned gpio_line = 17; // physical pin 11

    // real-time settings for cpu
    bool realtime = true; // --no-rt turns off RT settings
    int cpu = 3; // core
    int priority = 80; // SCHED_FIFO priority (1-99)

    // statistics
    uint64_t report_every = 1000; // report periodically every N packets
};

/**
* @brief Parses command-line arguments into a Config.
*
* Every option is optional; values not given on the command line keep
* the defaults from Config.
*
* Supported options:
*   -p, --port <n>          UDP port (1–65535)
*   -c, --chip <path>       GPIO chip device, e.g. /dev/gpiochip0
*   -l, --line <n>          GPIO line offset
*       --no-rt             disable real-time setup (affinity, SCHED_FIFO, mlockall)
*       --cpu <n>           CPU core to pin the process to
*       --prio <n>          SCHED_FIFO priority (1–99)
*   -r, --report-every <n>  print statistics every n packets
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
Config parse_args(int argc, char *argv[]);

#endif //RECIEVER_CONFIG_H