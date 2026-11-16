#ifndef LATENCY_RX_REALTIME_H
#define LATENCY_RX_REALTIME_H

/// Pins the process to @p cpu, locks all memory and switches to SCHED_FIFO @p priority.
/// Call after all allocations, right before the receive loop.
/// @throws std::system_error with a hint when privileges or limits are missing.
void setup_realtime(int cpu, int priority);

/// True if the kernel limits RT tasks to sched_rt_runtime_us per second (default 95 %).
/// A spinning SCHED_FIFO task is then paused when other tasks run on its CPU.
bool rt_throttling_enabled();

#endif // LATENCY_RX_REALTIME_H
