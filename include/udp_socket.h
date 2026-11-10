//
// Created by David Konečný on 09.11.2026.
//

#ifndef LATENCY_RX_UDP_SOCKET_H
#define LATENCY_RX_UDP_SOCKET_H

#include <sys/socket.h>
#include <sys/types.h>   // ssize_t
#include <cstddef>
#include <cstdint>

/// UDP socket bound to a port on all interfaces (RAII, closed in the destructor)
class UdpSocket {
public:
    static constexpr std::size_t kMaxDatagram = 2048;  // > MTU (1500 B)
    /// @throws std::system_error if the socket can't be created or bound
    ///         (port already in use, port < 1024 without CAP_NET_BIND_SERVICE)
    explicit UdpSocket(uint16_t port);
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    /// Blocks until a datagram arrives. Returns its length, or -1 with errno set.
    /// Inline: called on the hot path right before GpioToggle::toggle().
    ssize_t receive(void* buf, std::size_t len) { return ::recv(fd_, buf, len, 0); }
private:
    int fd_ = -1;
};
#endif //LATENCY_RX_UDP_SOCKET_H
