#include "recvmsg_socket.h"

#include <unistd.h>  // close

#include <cerrno>
#include <cstring>
#include <system_error>

RecvMsgSocket::RecvMsgSocket(int domain, int type, int protocol)
{
    fd_ = ::socket(domain, type, protocol);
    if (fd_ < 0) {
        const int err = errno;
        // EPERM here in practice only for AF_PACKET
        throw std::system_error(err, std::generic_category(),
                                err == EPERM ? "socket (needs root or CAP_NET_RAW)" : "socket");
    }

#ifdef SO_TIMESTAMPNS  // Linux only; elsewhere (macOS) the timestamp stays 0
    const int on = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_TIMESTAMPNS, &on, sizeof on) < 0) {
        const int err = errno;
        ::close(fd_);  // our own constructor failed: the destructor will not run
        throw std::system_error(err, std::generic_category(), "setsockopt SO_TIMESTAMPNS");
    }
#endif

    // set once; receive() only updates the data buffer and the control size
    msg_.msg_iov = &iov_;
    msg_.msg_iovlen = 1;
    msg_.msg_control = control_;
}

RecvMsgSocket::~RecvMsgSocket()
{
    ::close(fd_);
}

uint64_t RecvMsgSocket::last_rx_timestamp_ns()
{
#ifdef SO_TIMESTAMPNS
    for (cmsghdr* c = CMSG_FIRSTHDR(&msg_); c != nullptr; c = CMSG_NXTHDR(&msg_, c)) {
        if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SCM_TIMESTAMPNS) {
            timespec ts;
            std::memcpy(&ts, CMSG_DATA(c), sizeof ts);  // CMSG_DATA may be unaligned
            return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
                   + static_cast<uint64_t>(ts.tv_nsec);
        }
    }
#endif
    return 0;
}
