//
// Created by David Konečný on 10.11.2026.
//

#include "udp_socket.h"

#include <arpa/inet.h> // htons, htonl
#include <netinet/in.h> // sockaddr_in, INADDR_ANY
#include <unistd.h>       // close
#include <cerrno>
#include <cstring>
#include <string>
#include <system_error>

UdpSocket::UdpSocket(uint16_t port, int busy_poll_us)
{
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        throw std::system_error(errno, std::generic_category(), "socket");
    }

    // destructor will not run (constructor did not finish): close here
    auto fail = [this](const std::string& what) {
        const int err = errno;  // close() could overwrite errno
        ::close(fd_);
        throw std::system_error(err, std::generic_category(), what);
    };

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);    // network byte order
    addr.sin_addr.s_addr = htonl(INADDR_ANY); //all interfaces

    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0) {
        fail("bind UDP port " + std::to_string(port));
    }

#ifdef SO_TIMESTAMPNS  // Linux only; elsewhere (macOS) the timestamp stays 0
    const int on = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_TIMESTAMPNS, &on, sizeof on) < 0) {
        fail("setsockopt SO_TIMESTAMPNS");
    }
#endif

    if (busy_poll_us > 0) {
#ifdef SO_BUSY_POLL
        if (::setsockopt(fd_, SOL_SOCKET, SO_BUSY_POLL, &busy_poll_us, sizeof busy_poll_us) < 0) {
            fail(errno == EPERM ? "setsockopt SO_BUSY_POLL (needs root or CAP_NET_ADMIN)"
                                : "setsockopt SO_BUSY_POLL");
        }
#else
        errno = ENOTSUP;
        fail("SO_BUSY_POLL (Linux only)");
#endif
    }

    // set once; receive() only updates the data buffer and the control size
    msg_.msg_iov = &iov_;
    msg_.msg_iovlen = 1;
    msg_.msg_control = control_;
}

UdpSocket::~UdpSocket()
{
    ::close(fd_);
}

uint64_t UdpSocket::last_rx_timestamp_ns()
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

unsigned UdpSocket::napi_id() const
{
#ifdef SO_INCOMING_NAPI_ID
    unsigned id = 0;
    socklen_t len = sizeof id;
    if (::getsockopt(fd_, SOL_SOCKET, SO_INCOMING_NAPI_ID, &id, &len) == 0) {
        return id;
    }
#endif
    return 0;
}
