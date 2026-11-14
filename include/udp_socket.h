//
// Created by David Konečný on 09.11.2026.
//

#ifndef LATENCY_RX_UDP_SOCKET_H
#define LATENCY_RX_UDP_SOCKET_H

#include <sys/socket.h>
#include <sys/types.h>   // ssize_t
#include <sys/uio.h>     // iovec
#include <cstddef>
#include <cstdint>
#include <ctime>         // timespec

/// UDP socket bound to a port on all interfaces (RAII, closed in the destructor)
class UdpSocket {
public:
    static constexpr std::size_t kMaxDatagram = 2048;  // > MTU (1500 B)

    /// Binds @p port and enables kernel RX timestamps (SO_TIMESTAMPNS, Linux only).
    /// @throws std::system_error if the socket can't be created or bound
    ///         (port already in use, port < 1024 without CAP_NET_BIND_SERVICE)
    explicit UdpSocket(uint16_t port);
    ~UdpSocket();

    // msg_ points into this object: copying or moving would leave dangling pointers
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    /// Blocks until a datagram arrives. Returns its length, or -1 with errno set.
    /// Inline: called on the hot path right before GpioToggle::toggle().
    ssize_t receive(void* buf, std::size_t len)
    {
        iov_ = {buf, len};
        msg_.msg_controllen = sizeof control_;  // in: space, out: used size → reset each call
        return ::recvmsg(fd_, &msg_, 0);
    }

    /// Kernel RX timestamp of the last datagram (CLOCK_REALTIME ns), 0 if missing.
    /// Call after the GPIO toggle: parsing the control messages is not free.
    uint64_t last_rx_timestamp_ns();

private:
    int fd_ = -1;
    iovec iov_{};
    msghdr msg_{};
    alignas(cmsghdr) unsigned char control_[CMSG_SPACE(sizeof(timespec))];
};
#endif //LATENCY_RX_UDP_SOCKET_H
