#include "realtime.h"

#include <sched.h>
#include <sys/mman.h>

#include <cerrno>
#include <cstddef>
#include <string>
#include <system_error>

namespace {

[[noreturn]] void fail(const std::string& what)
{
    const int err = errno;
    std::string hint;
    if (err == EPERM) {
        hint = " (run as root, or: sudo setcap cap_sys_nice,cap_ipc_lock+ep latency_rx; or --no-rt)";
    } else if (err == ENOMEM) {
        hint = " (RLIMIT_MEMLOCK too low: needs CAP_IPC_LOCK, or a smaller --samples)";
    } else if (err == EINVAL) {
        hint = " (no such CPU, see nproc)";
    }
    throw std::system_error(err, std::generic_category(), what + hint);
}

// Stack growth in the loop would page-fault; touch the pages now (mlockall keeps them)
void prefault_stack()
{
    volatile unsigned char dummy[64 * 1024];
    for (std::size_t i = 0; i < sizeof dummy; i += 4096) {
        dummy[i] = 0;
    }
}

} // namespace

void setup_realtime(int cpu, int priority)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof set, &set) != 0) {
        fail("sched_setaffinity(cpu " + std::to_string(cpu) + ")");
    }

    // CURRENT: already mapped pages (incl. SampleLog), FUTURE: later allocations
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        fail("mlockall");
    }
    prefault_stack();

    sched_param sp{};
    sp.sched_priority = priority;
    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
        fail("sched_setscheduler(SCHED_FIFO " + std::to_string(priority) + ")");
    }
}
