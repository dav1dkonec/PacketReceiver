//
// Created by David Konečný on 27.10.2026.
//

#include "config.h"
#include <getopt.h>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <array>
#include <utility>

namespace {

// Long options without a short variant need a value outside the char range,
// so they cannot collide with 'p', 'c', ...
enum LongOnlyOption : int {
    OPT_NO_RT = 256,
    OPT_CPU,
    OPT_PRIO,
    OPT_BUSY_POLL_US,
    OPT_SAMPLES,
};

// Single source of truth for mode names: used by parsing, help and to_string().
constexpr std::array<std::pair<RxMode, std::string_view>, 5> kModeNames{{
    {RxMode::Udp,      "udp"},
    {RxMode::BusyPoll, "busy-poll"},
    {RxMode::BusySpin, "busy-spin"},
    {RxMode::Packet,   "packet"},
    {RxMode::Xdp,      "xdp"},
}};

// Parses the whole string as a number in [min, max].
// "12x", "", "-1" for unsigned types, or out-of-range values all throw.
template <typename T>
T parse_number(std::string_view text, std::string_view option, T min, T max)
{
    T value{};
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, value);

    if (ec != std::errc{} || ptr != end || value < min || value > max)
    {
        throw std::invalid_argument(
            std::string(option) + ": invalid value '" + std::string(text) +
            "' (expected " + std::to_string(min) + "-" + std::to_string(max) + ")");
    }
    return value;
}

RxMode parse_mode(std::string_view text)
{
    for (const auto& [mode, name] : kModeNames) {
        if (text == name) {
            return mode;
        }
    }

    std::string valid;
    for (const auto& [mode, name] : kModeNames) {
        valid += valid.empty() ? "" : ", ";
        valid += name;
    }
    throw std::invalid_argument("--mode: unknown mode '" + std::string(text) +
                                "' (expected one of: " + valid + ")");
}

std::string non_empty(const char* text, std::string_view option)
{
    if (*text == '\0') {
        throw std::invalid_argument(std::string(option) + ": empty value");
    }
    return text;
}


void print_usage(const char* prog)
{
    const Config def;  // defaults are taken from Config, so help never goes stale
    std::cout
        << "Usage: " << prog << " [options]\n"
        << "\nNetwork:\n"
        << "  -p, --port <n>          UDP port (default " << def.port << ")\n"
        << "  -i, --interface <name>  network interface (default " << def.interface << ")\n"
        << "  -m, --mode <mode>       udp | busy-poll | busy-spin | packet | xdp (default "
        << to_string(def.mode) << ")\n"
        << "      --busy-poll-us <n>  SO_BUSY_POLL budget in us (default " << def.busy_poll_us << ")\n"
        << "\nGPIO:\n"
        << "  -c, --chip <path>       GPIO chip device (default " << def.gpio_chip << ")\n"
        << "  -l, --line <n>          GPIO line offset (default " << def.gpio_line << ")\n"
        << "\nReal-time:\n"
        << "      --no-rt             disable real-time setup\n"
        << "      --cpu <n>           CPU core to pin to (default " << def.cpu << ")\n"
        << "      --prio <n>          SCHED_FIFO priority 1-99 (default " << def.priority << ")\n"
        << "\nResults:\n"
        << "  -r, --report-every <n>  print statistics every n packets (default "
        << def.report_every << ")\n"
        << "  -o, --output <file>     save samples to CSV at exit (default: none)\n"
        << "      --samples <n>       sample buffer capacity (default " << def.max_samples << ")\n"
        << "\n  -h, --help              print this help and exit\n";
}

} // namespace

std::string_view to_string(RxMode mode)
{
    for (const auto& [m, name] : kModeNames) {
        if (m == mode) {
            return name;
        }
    }
    return "?";
}

Config parse_args(int argc, char *argv[])
{
    Config cfg;

    static const option long_options[] = {
        {"port",          required_argument, nullptr, 'p'},
        {"interface",     required_argument, nullptr, 'i'},
        {"mode",          required_argument, nullptr, 'm'},
        {"busy-poll-us",  required_argument, nullptr, OPT_BUSY_POLL_US},
        {"chip",          required_argument, nullptr, 'c'},
        {"line",          required_argument, nullptr, 'l'},
        {"no-rt",         no_argument,       nullptr, OPT_NO_RT},
        {"cpu",           required_argument, nullptr, OPT_CPU},
        {"prio",          required_argument, nullptr, OPT_PRIO},
        {"report-every",  required_argument, nullptr, 'r'},
        {"output",        required_argument, nullptr, 'o'},
        {"samples",       required_argument, nullptr, OPT_SAMPLES},
        {"help",          no_argument,       nullptr, 'h'},
        {nullptr,         0,                 nullptr, 0},  // end of array
    };

    // Leading ':' → getopt returns ':' for a missing value and '?' for
    // an unknown option, so the two cases can be reported differently.
    const char* short_options = ":p:i:m:c:l:r:o:h";

    // getopt keeps its state in globals; reset it so parse_args can be
    // called repeatedly (unit tests)
#ifdef __GLIBC__
    optind = 0;    // glibc: 0 = full reset
#else
    optreset = 1;  // BSD / macOS
    optind = 1;
#endif
    opterr = 0;  // errors are reported via exceptions
    int opt;
    while ((opt = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (opt) {
        case 'p':
            cfg.port = parse_number<uint16_t>(optarg, "--port", 1, 65535);
            break;
        case 'i':
            cfg.interface = non_empty(optarg, "--interface");
            break;
        case 'm':
            cfg.mode = parse_mode(optarg);
            break;
        case OPT_BUSY_POLL_US:
            cfg.busy_poll_us = parse_number<int>(optarg, "--busy-poll-us", 1, 1'000'000);
            break;
        case 'c':
            cfg.gpio_chip = non_empty(optarg, "--chip");
            break;
        case 'l':
            cfg.gpio_line = parse_number<unsigned>(
                optarg, "--line", 0, std::numeric_limits<unsigned>::max());
            break;
        case OPT_NO_RT:
            cfg.realtime = false;
            break;
        case OPT_CPU:
            // 1023 = CPU_SETSIZE - 1; whether the core exists is checked
            // later by sched_setaffinity()
            cfg.cpu = parse_number<int>(optarg, "--cpu", 0, 1023);
            break;
        case OPT_PRIO:
            cfg.priority = parse_number<int>(optarg, "--prio", 1, 99);
            break;
        case 'r':
            // 0 is rejected: received % report_every would divide by zero
            cfg.report_every = parse_number<uint32_t>(
                optarg, "--report-every", 1, std::numeric_limits<uint32_t>::max());
            break;
        case 'o':
            cfg.output_csv = non_empty(optarg, "--output");
            break;
        case OPT_SAMPLES:
            // upper bound keeps the locked buffer in the order of hundreds of MB
            cfg.max_samples = parse_number<std::size_t>(optarg, "--samples", 1, 10'000'000);
            break;
        case 'h':
            print_usage(argv[0]);
            std::exit(EXIT_SUCCESS);
        case ':':
            // missing value is always the last argument → argv[optind - 1]
            throw std::invalid_argument(std::string(argv[optind - 1]) + ": missing value");
        default:  // '?'
            if (optopt != 0) {  // unknown short option
                throw std::invalid_argument(std::string("unknown option: -") +
                                            static_cast<char>(optopt));
            }
            throw std::invalid_argument(std::string("unknown option: ") + argv[optind - 1]);
        }
    }

    if (optind < argc) {
        throw std::invalid_argument(std::string("unexpected argument: ") + argv[optind]);
    }
    return cfg;
}