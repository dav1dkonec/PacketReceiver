#ifndef LATENCY_RX_SPIN_UDP_SOCKET_H
#define LATENCY_RX_SPIN_UDP_SOCKET_H

#include <sys/socket.h>  // MSG_DONTWAIT
#include <sys/types.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "stop_signal.h"
#include "udp_socket.h"

/// busy-spin mode: polls the UDP socket with non-blocking recv in a tight loop.
/// The process never sleeps, so no wake-up sits between the packet and the toggle;
/// the core runs at 100 %.
class SpinUdpSocket {
public:
    SpinUdpSocket(uint16_t port, const std::string& address) : sock_(port, address) {}

    /// Spins until a datagram arrives or g_stop is set (then -1).
    ssize_t receive(void* buf, std::size_t len)
    {
        ssize_t n;
        // errno is read only while spinning, never between a success and the toggle
        do {
            n = sock_.receive(buf, len, MSG_DONTWAIT);
        } while (n < 0 && errno == EAGAIN && !g_stop);
        return n;
    }

    uint64_t last_rx_timestamp_ns() { return sock_.last_rx_timestamp_ns(); }

    static std::span<const unsigned char> payload(const unsigned char* buf, std::size_t n)
    {
        return UdpSocket::payload(buf, n);
    }

    unsigned napi_id() const { return sock_.napi_id(); }

private:
    UdpSocket sock_;
};

#endif // LATENCY_RX_SPIN_UDP_SOCKET_H
