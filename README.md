# latency_rx

Test packet receiver for Raspberry Pi, used to measure network stack latency.

When a packet arrives, the program immediately toggles a GPIO output. An external
probe (Pico) measures the time between the frame arriving on the wire and this edge.
No code may run between `recv()` and the GPIO toggle: checks, statistics and output
all happen after the toggle.

## Requirements (Raspberry Pi)

```bash
sudo apt install build-essential cmake pkg-config libgpiod-dev gpiod
sudo usermod -aG gpio $USER     # access to /dev/gpiochip* without root
```

- libgpiod **v2** (`libgpiodcxx >= 2.0`)
- UDP port allowed in the firewall, e.g. `udp dport 5000 accept`

## Build

```bash
cmake -S . -B build
cmake --build build -j4
```

The default build type is `Release`. On macOS only the portable `rxcore` library
(configuration) is built; the receiver itself needs Linux and libgpiod.

## Usage

```bash
./build/latency_rx                # no arguments: print usage (same as --help)
sudo ./build/latency_rx --mode udp --cpu 3 --fifo-prio 80 -o run.csv
./build/latency_rx --no-rt        # measure without real-time setup
```

Main options:

| option | meaning | default |
|---|---|---|
| `-p, --port` | UDP port of the test packets | 5000 |
| `-m, --mode` | `udp`, `busy-poll`, `busy-spin`, `packet`, `xdp` | `udp` |
| `-b, --busy-poll-us` | SO_BUSY_POLL budget in microseconds (busy-poll mode) | 50 |
| `-g, --gpio-chip`, `-l, --line` | GPIO chip and line | `/dev/gpiochip0`, 17 (pin 11) |
| `-n, --no-rt`, `-c, --cpu`, `-f, --fifo-prio` | real-time setup (affinity, SCHED_FIFO, mlockall) | on, CPU 3, 80 |
| `-r, --report-every` | print statistics every N packets | 1000 |
| `-o, --output`, `-s, --samples` | CSV with samples, written at the end of the run | none, 1 000 000 |

Real-time setup requires root, or the `CAP_SYS_NICE` and `CAP_IPC_LOCK` capabilities.
Without them the program exits with an error. Use `--no-rt` to measure without RT.

## Layout

```
include/   module headers (config.h, ...)
src/       implementation and main.cpp
tools/     helper programs (gpio_bench)
```

## Status

- [x] configuration and command-line options
- [ ] GPIO output
- [ ] UDP socket receive, statistics
- [ ] shutdown (SIGINT) and CSV output
- [ ] kernel software timestamps (`SO_TIMESTAMPNS`)
- [ ] real-time setup
- [ ] additional receive modes (busy poll, AF_PACKET, optionally XDP)
