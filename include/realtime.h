#ifndef LATENCY_RX_REALTIME_H
#define LATENCY_RX_REALTIME_H

/// Pins the process to @p cpu, locks all memory and switches to SCHED_FIFO @p priority.
/// Call after all allocations, right before the receive loop.
/// @throws std::system_error with a hint when privileges or limits are missing.
void setup_realtime(int cpu, int priority);

#endif // LATENCY_RX_REALTIME_H
