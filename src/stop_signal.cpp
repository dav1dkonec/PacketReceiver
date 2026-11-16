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
    sa.sa_flags = 0;  // no SA_RESTART
    // kill, timeout (automation script): may arrive twice, timeout(1) signals
    // the child and its process group, so a 2nd SIGTERM must not kill
    sigaction(SIGTERM, &sa, nullptr);
    sa.sa_flags = SA_RESETHAND;  // Ctrl+C: a 2nd one kills
    sigaction(SIGINT, &sa, nullptr);
}
