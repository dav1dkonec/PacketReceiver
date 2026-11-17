//
// Created by David Konečný on 09.11.2026.
//

#ifndef LATENCY_RX_UDP_SOCKET_H
#define LATENCY_RX_UDP_SOCKET_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "recvmsg_socket.h"

/// UDP socket bound to a port (udp, busy-poll and busy-spin modes).
class UdpSocket : public RecvMsgSocket {
public:
    /// Binds @p address:@p port; empty @p address = all interfaces (INADDR_ANY).
    /// @p busy_poll_us > 0 sets SO_BUSY_POLL: a blocking receive() polls the NIC
    /// for up to that long before sleeping.
    /// @throws std::system_error if the socket can't be created, bound or configured
    ///         (port already in use, address not on this host,
    ///         port < 1024 without CAP_NET_BIND_SERVICE)
    explicit UdpSocket(uint16_t port, const std::string& address = {}, int busy_poll_us = 0);

    /// The whole datagram is the payload (the kernel already stripped IP/UDP headers).
    static std::span<const unsigned char> payload(const unsigned char* buf, std::size_t n)
    {
        return {buf, n};
    }

    /// NAPI instance of the last received packet (SO_INCOMING_NAPI_ID), 0 if
    /// unknown. 0 in busy-poll mode means busy polling could not work.
    unsigned napi_id() const;
};
#endif //LATENCY_RX_UDP_SOCKET_H
