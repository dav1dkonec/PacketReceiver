#ifndef LATENCY_RX_RECVMSG_SOCKET_H
#define LATENCY_RX_RECVMSG_SOCKET_H

#include <sys/socket.h>
#include <sys/types.h>  // ssize_t
#include <sys/uio.h>    // iovec

#include <cstddef>
#include <cstdint>
#include <ctime>        // timespec

/// Common part of UdpSocket and PacketSocket: owns the fd (RAII), enables kernel
/// RX timestamps and receives with recvmsg(). Subclasses only bind and configure.
/// Not virtual: run_loop is a template, nothing is called through a base pointer.
class RecvMsgSocket {
public:
    // msg_ points into this object: copying or moving would leave dangling pointers
    RecvMsgSocket(const RecvMsgSocket&) = delete;
    RecvMsgSocket& operator=(const RecvMsgSocket&) = delete;

    /// Blocks until a packet arrives (or with MSG_DONTWAIT fails with EAGAIN).
    /// Returns its length, or -1 with errno set.
    /// Inline: called on the hot path right before GpioToggle::toggle().
    ssize_t receive(void* buf, std::size_t len, int flags = 0)
    {
        iov_ = {buf, len};
        msg_.msg_controllen = sizeof control_;  // in: space, out: used size → reset each call
        return ::recvmsg(fd_, &msg_, flags);
    }

    /// Kernel RX timestamp of the last packet (SO_TIMESTAMPNS, CLOCK_REALTIME ns),
    /// 0 if missing (macOS). Call after the GPIO toggle: parsing is not free.
    uint64_t last_rx_timestamp_ns();

protected:
    /// Creates the socket and enables SO_TIMESTAMPNS (Linux only).
    /// If a subclass constructor throws later, ~RecvMsgSocket still closes the fd.
    /// @throws std::system_error
    RecvMsgSocket(int domain, int type, int protocol);

    // protected and not virtual: only subclasses destroy it, deleting through
    // a RecvMsgSocket* does not compile (it would skip the subclass destructor)
    ~RecvMsgSocket();

    int fd() const { return fd_; }

private:
    int fd_ = -1;
    iovec iov_{};
    msghdr msg_{};
    alignas(cmsghdr) unsigned char control_[CMSG_SPACE(sizeof(timespec))];
};

#endif // LATENCY_RX_RECVMSG_SOCKET_H
