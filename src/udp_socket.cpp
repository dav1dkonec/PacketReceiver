//
// Created by David Konečný on 10.11.2026.
//

#include "udp_socket.h"

#include <arpa/inet.h> // htons, htonl, inet_pton
#include <netinet/in.h> // sockaddr_in, INADDR_ANY
#include <sys/socket.h>
#include <cerrno>
#include <string>
#include <system_error>

UdpSocket::UdpSocket(uint16_t port, const std::string& address, int busy_poll_us)
    : RecvMsgSocket(AF_INET, SOCK_DGRAM, 0)
{
    // a throw below still closes the fd: the base class is already constructed

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);    // network byte order
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // all interfaces
    if (!address.empty() && ::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
        throw std::system_error(EINVAL, std::generic_category(), "address " + address);
    }

    if (::bind(fd(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0) {
        const int err = errno;  // before building the message: allocation may change errno
        const std::string where = (address.empty() ? "*" : address) + ":" + std::to_string(port);
        throw std::system_error(err, std::generic_category(),
                                err == EADDRNOTAVAIL ? "bind " + where + " (address not on this host)"
                                                     : "bind " + where);
    }

    if (busy_poll_us > 0) {
#ifdef SO_BUSY_POLL
        if (::setsockopt(fd(), SOL_SOCKET, SO_BUSY_POLL, &busy_poll_us, sizeof busy_poll_us) < 0) {
            const int err = errno;
            throw std::system_error(err, std::generic_category(),
                                    err == EPERM ? "setsockopt SO_BUSY_POLL (needs root or CAP_NET_ADMIN)"
                                                 : "setsockopt SO_BUSY_POLL");
        }
#else
        throw std::system_error(ENOTSUP, std::generic_category(), "SO_BUSY_POLL (Linux only)");
#endif
    }
}

unsigned UdpSocket::napi_id() const
{
#ifdef SO_INCOMING_NAPI_ID
    unsigned id = 0;
    socklen_t len = sizeof id;
    if (::getsockopt(fd(), SOL_SOCKET, SO_INCOMING_NAPI_ID, &id, &len) == 0) {
        return id;
    }
#endif
    return 0;
}
