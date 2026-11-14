#ifndef LATENCY_RX_SAMPLE_LOG_H
#define LATENCY_RX_SAMPLE_LOG_H

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

/// One received datagram.
struct Sample {
    int64_t  seq;       // sequence number, -1 = invalid datagram
    uint64_t user_ns;   // CLOCK_REALTIME right after the GPIO toggle
};

/// Fixed-size sample buffer: all memory is allocated up front,
/// add() never allocates, the CSV is written only after the run.
class SampleLog {
public:
    /// Allocates and zero-fills @p capacity samples: page faults happen here,
    /// not in the receive loop.
    explicit SampleLog(std::size_t capacity) : samples_(capacity) {}

    /// Stores a sample; when the buffer is full the sample is only counted as dropped.
    void add(const Sample& s)
    {
        if (count_ < samples_.size()) {
            samples_[count_++] = s;
        } else {
            ++dropped_;
        }
    }

    std::size_t size() const { return count_; }
    std::size_t dropped() const { return dropped_; }

    /// Writes "index,seq,user_ns" lines and flushes @p out.
    /// @throws std::runtime_error if writing fails (e.g. disk full).
    void write_csv(std::ostream& out) const;

private:
    std::vector<Sample> samples_;
    std::size_t count_ = 0;
    std::size_t dropped_ = 0;
};

#endif // LATENCY_RX_SAMPLE_LOG_H
