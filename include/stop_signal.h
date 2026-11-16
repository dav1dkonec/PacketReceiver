#ifndef LATENCY_RX_STOP_SIGNAL_H
#define LATENCY_RX_STOP_SIGNAL_H

#include <csignal>

/// Set to 1 by SIGINT/SIGTERM; receive loops poll it.
/// sig_atomic_t: written atomically; volatile: re-read on every check.
extern volatile std::sig_atomic_t g_stop;

/// Installs the SIGINT/SIGTERM handler that sets g_stop.
/// Without SA_RESTART, so a blocked recv() returns EINTR; a second SIGINT kills.
void install_stop_handler();

#endif // LATENCY_RX_STOP_SIGNAL_H
