//
// Created by David Konečný on 10.11.2026.
//

#include "udp_socket.h"

#include <arpa/inet.h> // htons, htonl
#include <netinet/in.h> // sockaddr_in, INADDR_ANY
#include <unistd.h>       // close
#include <cerrno>
#include <string>
#include <system_error>

UdpSocket::UdpSocket(uint16_t port)
{
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        throw std::system_error(errno, std::generic_category(), "socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);    // network byte order
    addr.sin_addr.s_addr = htonl(INADDR_ANY); //all interfaces

    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0) {
        const int err = errno; // close() could overwrite errno
        ::close(fd_);          // destructor will not run: constructor did not finish
        throw std::system_error(err, std::generic_category(), "bind UDP port " + std::to_string(port));
    }
}

UdpSocket::~UdpSocket()
{
    ::close(fd_);
}
