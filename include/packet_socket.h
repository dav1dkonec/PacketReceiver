#ifndef LATENCY_RX_PACKET_SOCKET_H
#define LATENCY_RX_PACKET_SOCKET_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "recvmsg_socket.h"

/// packet mode: AF_PACKET socket on one interface, Linux only.
/// Receives IPv4 packets right after the driver (__netif_receive_skb), before
/// the IP/UDP stack and netfilter. A kernel BPF filter passes only unicast
/// UDP to @p port, so nothing foreign wakes the loop or toggles the GPIO.
/// The IP stack still processes the packet too (no listener → ICMP port unreachable).
class PacketSocket : public RecvMsgSocket {
public:
    /// @throws std::system_error (no such interface, missing CAP_NET_RAW, ...)
    PacketSocket(const std::string& interface, uint16_t port);

    // receive() (buf starts at the IPv4 header) and last_rx_timestamp_ns()
    // come from RecvMsgSocket

    /// UDP payload behind the IPv4 and UDP headers, empty if malformed.
    static std::span<const unsigned char> payload(const unsigned char* buf, std::size_t n)
    {
        if (n < 20) {
            return {};
        }
        const std::size_t ihl = (buf[0] & 0x0fu) * 4u;  // IHL: header length in 32-bit words
        if (ihl < 20 || n < ihl + 8) {
            return {};
        }
        // UDP length, not n: AF_PACKET sees frames before IP trims Ethernet padding
        const std::size_t udp_len = (std::size_t{buf[ihl + 4]} << 8) | buf[ihl + 5];
        if (udp_len < 8 || ihl + udp_len > n) {
            return {};
        }
        return {buf + ihl + 8, udp_len - 8};
    }
};

#endif // LATENCY_RX_PACKET_SOCKET_H
