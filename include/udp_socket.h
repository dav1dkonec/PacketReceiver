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
#include <span>

/// UDP socket bound to a port on all interfaces (RAII, closed in the destructor)
class UdpSocket {
public:
    /// Binds @p port and enables kernel RX timestamps (SO_TIMESTAMPNS, Linux only).
    /// @p busy_poll_us > 0 sets SO_BUSY_POLL: a blocking receive() polls the NIC
    /// for up to that long before sleeping (raising it needs CAP_NET_ADMIN).
    /// @throws std::system_error if the socket can't be created, bound or configured
    ///         (port already in use, port < 1024 without CAP_NET_BIND_SERVICE)
    explicit UdpSocket(uint16_t port, int busy_poll_us = 0);
    ~UdpSocket();

    // msg_ points into this object: copying or moving would leave dangling pointers
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    /// Blocks until a datagram arrives (or with MSG_DONTWAIT fails with EAGAIN).
    /// Returns its length, or -1 with errno set.
    /// Inline: called on the hot path right before GpioToggle::toggle().
    ssize_t receive(void* buf, std::size_t len, int flags = 0)
    {
        iov_ = {buf, len};
        msg_.msg_controllen = sizeof control_;  // in: space, out: used size → reset each call
        return ::recvmsg(fd_, &msg_, flags);
    }

    /// The whole datagram is the payload (the kernel already stripped IP/UDP headers).
    static std::span<const unsigned char> payload(const unsigned char* buf, std::size_t n)
    {
        return {buf, n};
    }

    /// Kernel RX timestamp of the last datagram (CLOCK_REALTIME ns), 0 if missing.
    /// Call after the GPIO toggle: parsing the control messages is not free.
    uint64_t last_rx_timestamp_ns();

    /// NAPI instance of the last received packet (SO_INCOMING_NAPI_ID), 0 if
    /// unknown. 0 in busy-poll mode means busy polling could not work.
    unsigned napi_id() const;

private:
    int fd_ = -1;
    iovec iov_{};
    msghdr msg_{};
    alignas(cmsghdr) unsigned char control_[CMSG_SPACE(sizeof(timespec))];
};
#endif //LATENCY_RX_UDP_SOCKET_H
