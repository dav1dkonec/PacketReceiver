//
// Created by David Konečný on 10.11.2026.
//

#include "stats.h"
#include "packet.h"

#include <algorithm>
#include <cstring>
#include <iomanip>

int64_t Stats::on_packet(const unsigned char* data, std::size_t len)
{
    ++received;

    test_packet pkt;
    if (len != sizeof pkt) {
        ++invalid;
        return -1;
    }

    std::memcpy(&pkt, data, sizeof pkt); // no alignment / aliasing issues

    if (tp_ntoh32(pkt.magic) != TEST_PACKET_MAGIC) {
        ++invalid;
        return -1;
    }

    const uint32_t seq = tp_ntoh32(pkt.seq);
    // a late packet is counted in both lost and reordered
    if (started_ && seq < next_seq_) {
        ++reordered;
        return seq;
    }

    if (started_) {
        lost += seq - next_seq_; // 0 when seq == next_seq_
    }

    started_ = true;
    next_seq_ = seq + 1;
    return seq;
}

void Stats::on_latency(uint64_t kernel_ns, uint64_t user_ns)
{
    if (kernel_ns == 0 || kernel_ns > user_ns) {
        return;
    }
    const uint64_t lat = user_ns - kernel_ns;
    ++lat_count_;
    lat_sum_ += lat;
    lat_min_ = std::min(lat_min_, lat);
    lat_max_ = std::max(lat_max_, lat);
}

void Stats::print(std::ostream &out) const {
    out << "received " << received << ", invalid " << invalid
    << ", lost " << lost << ", reordered " << reordered;
    if (lat_count_ != 0) {
        const double avg = static_cast<double>(lat_sum_) / static_cast<double>(lat_count_);
        out << std::fixed << std::setprecision(1)
            << ", k->u us min " << static_cast<double>(lat_min_) / 1e3
            << " avg " << avg / 1e3
            << " max " << static_cast<double>(lat_max_) / 1e3;
    }
    out << '\n';
}
