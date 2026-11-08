// Unit tests for parse_args() – no framework, run with: ctest --test-dir build

#include "config.h"

#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(expr)                                                         \
    do {                                                                    \
        if (!(expr)) {                                                      \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: "  \
                      << #expr << "\n";                                     \
            ++g_failures;                                                   \
        }                                                                   \
    } while (0)

// Calls parse_args with a real, writable argv (getopt may reorder it).
Config parse(std::initializer_list<const char*> args)
{
    std::vector<std::string> storage{"latency_rx"};
    storage.insert(storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    for (auto& s : storage) {
        argv.push_back(s.data());
    }
    argv.push_back(nullptr);

    return parse_args(static_cast<int>(storage.size()), argv.data());
}

// Expects std::invalid_argument whose message contains `expected`.
void check_throws(std::initializer_list<const char*> args, const std::string& expected, int line)
{
    try {
        parse(args);
        std::cerr << __FILE__ << ":" << line << ": expected exception containing '"
                  << expected << "', nothing thrown\n";
        ++g_failures;
    } catch (const std::invalid_argument& e) {
        if (std::string(e.what()).find(expected) == std::string::npos) {
            std::cerr << __FILE__ << ":" << line << ": wrong message: '" << e.what()
                      << "', expected to contain '" << expected << "'\n";
            ++g_failures;
        }
    }
}

#define CHECK_THROWS(expected, ...) check_throws({__VA_ARGS__}, expected, __LINE__)

void test_defaults()
{
    const Config c = parse({});
    const Config def;
    CHECK(c.port == def.port);
    CHECK(c.interface == def.interface);
    CHECK(c.mode == def.mode);
    CHECK(c.realtime == def.realtime);
    CHECK(c.cpu == def.cpu);
    CHECK(c.output_csv.empty());
}

void test_short_options()
{
    const Config c = parse({"-p", "6000", "-i", "eth1", "-m", "packet", "-g", "/dev/gpiochip4",
                            "-l", "22", "-r", "500", "-o", "run.csv"});
    CHECK(c.port == 6000);
    CHECK(c.interface == "eth1");
    CHECK(c.mode == RxMode::Packet);
    CHECK(c.gpio_chip == "/dev/gpiochip4");
    CHECK(c.gpio_line == 22);
    CHECK(c.report_every == 500);
    CHECK(c.output_csv == "run.csv");

    const Config rt = parse({"-b", "200", "-n", "-c", "2", "-f", "90", "-s", "5000"});
    CHECK(rt.busy_poll_us == 200);
    CHECK(!rt.realtime);
    CHECK(rt.cpu == 2);
    CHECK(rt.priority == 90);
    CHECK(rt.max_samples == 5000);

    // value glued to the option (-p6000) and a flag grouped with an option (-nc2)
    const Config glued = parse({"-p6000", "-nc2"});
    CHECK(glued.port == 6000);
    CHECK(!glued.realtime);
    CHECK(glued.cpu == 2);
}

void test_long_options()
{
    const Config c = parse({"--port", "7000", "--mode=busy-poll", "--busy-poll-us", "200",
                            "--no-rt", "--cpu", "2", "--fifo-prio", "90", "--samples", "5000"});
    CHECK(c.port == 7000);
    CHECK(c.mode == RxMode::BusyPoll);
    CHECK(c.busy_poll_us == 200);
    CHECK(!c.realtime);
    CHECK(c.cpu == 2);
    CHECK(c.priority == 90);
    CHECK(c.max_samples == 5000);
}

void test_abbreviations()
{
    // unambiguous prefixes of long options are accepted by getopt_long
    CHECK(parse({"--p", "6000"}).port == 6000);
    CHECK(parse({"--p=6001"}).port == 6001);
    CHECK(parse({"--f", "90"}).priority == 90);
}

void test_all_modes()
{
    for (RxMode mode : {RxMode::Udp, RxMode::BusyPoll, RxMode::BusySpin, RxMode::Packet,
                        RxMode::Xdp}) {
        const std::string name(to_string(mode));
        CHECK(parse({"--mode", name.c_str()}).mode == mode);
    }
}

void test_range_limits()
{
    CHECK(parse({"-p", "1"}).port == 1);
    CHECK(parse({"-p", "65535"}).port == 65535);
    CHECK(parse({"--fifo-prio", "1"}).priority == 1);
    CHECK(parse({"--fifo-prio", "99"}).priority == 99);

    CHECK_THROWS("--port", "-p", "0");
    CHECK_THROWS("--port", "-p", "65536");
    CHECK_THROWS("--fifo-prio", "--fifo-prio", "0");
    CHECK_THROWS("--fifo-prio", "--fifo-prio", "100");
    CHECK_THROWS("--cpu", "--cpu", "-1");
    CHECK_THROWS("--report-every", "-r", "0");
    CHECK_THROWS("--samples", "--samples", "0");
}

void test_invalid_values()
{
    CHECK_THROWS("invalid value '12x'", "-p", "12x");
    CHECK_THROWS("invalid value ''", "-p", "");
    CHECK_THROWS("invalid value '-3'", "-l", "-3");
    CHECK_THROWS("unknown mode 'foo'", "-m", "foo");
    CHECK_THROWS("--interface", "-i", "");
    CHECK_THROWS("--output", "-o", "");
}

void test_bad_usage()
{
    CHECK_THROWS("missing value", "--fifo-prio");
    CHECK_THROWS("missing value", "-p");
    CHECK_THROWS("unknown option: --foo", "--foo");
    CHECK_THROWS("unknown option: -x", "-x");
    CHECK_THROWS("unknown option: --no-rt=1", "--no-rt=1");
    CHECK_THROWS("unexpected argument: extra", "extra");
}

void test_repeated_calls()
{
    // getopt state must not leak between calls
    CHECK(parse({"-p", "6000"}).port == 6000);
    CHECK(parse({"-p", "6001"}).port == 6001);
    CHECK(parse({}).port == Config{}.port);
}

} // namespace

int main()
{
    test_defaults();
    test_short_options();
    test_long_options();
    test_abbreviations();
    test_all_modes();
    test_range_limits();
    test_invalid_values();
    test_bad_usage();
    test_repeated_calls();

    if (g_failures != 0) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all config tests passed\n";
    return 0;
}
