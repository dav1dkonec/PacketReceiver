#include "stop_signal.h"

volatile std::sig_atomic_t g_stop = 0;

namespace {

void on_signal(int)
{
    // only async-signal-safe work here: the signal may interrupt malloc or cout
    g_stop = 1;
}

} // namespace

void install_stop_handler()
{
    struct sigaction sa{};  // "struct": the function sigaction() has the same name
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;  // no SA_RESTART; 2nd Ctrl+C kills
    sigaction(SIGINT, &sa, nullptr);   // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr);  // kill, timeout (automation script)
}
