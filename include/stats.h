//
// Created by David Konečný on 10.11.2026.
//

#ifndef LATENCY_RX_STATS_H
#define LATENCY_RX_STATS_H

#include <cstddef>
#include <cstdint>
#include <ostream>

/// Packet counters, updated after the GPIO toggle (never on the hot path).
struct Stats {
  uint64_t received  = 0;   // every datagram, valid or not (= number of GPIO edges)
  uint64_t invalid   = 0;   // wrong size or magic
  uint64_t lost      = 0;   // gaps in seq
  uint64_t reordered = 0;   // seq lower than expected (late or duplicate)

  /// Validates one datagram and updates the counters.
  void on_packet(const unsigned char* data, std::size_t len);

  void print(std::ostream& out) const;

private:
  uint32_t next_seq_ = 0;
  bool started_ = false;    // first valid packet sets next_seq_
};
#endif //LATENCY_RX_STATS_H
