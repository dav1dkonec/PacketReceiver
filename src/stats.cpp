//
// Created by David Konečný on 10.11.2026.
//

#include "stats.h"
#include "packet.h"

#include <cstring>

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

void Stats::print(std::ostream &out) const {
    out << "received " << received << ", invalid " << invalid
    << ", lost " << lost << ", reordered " << reordered << '\n';
}
