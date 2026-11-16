#include "packet_socket.h"

#include <arpa/inet.h>       // htons
#include <linux/filter.h>    // sock_filter, sock_fprog, BPF_*, SKF_AD_*
#include <linux/if_ether.h>  // ETH_P_IP
#include <net/if.h>          // if_nametoindex
#include <netinet/in.h>      // IPPROTO_UDP
#include <netpacket/packet.h>  // sockaddr_ll, PACKET_HOST

#include <cerrno>
#include <system_error>

PacketSocket::PacketSocket(const std::string& interface, uint16_t port)
    // SOCK_DGRAM: link-layer header stripped, buf starts at the IP header.
    // Protocol 0: receives nothing until bind(), so no unfiltered packet is queued.
    : RecvMsgSocket(AF_PACKET, SOCK_DGRAM, 0)
{
    // a throw below still closes the fd: the base class is already constructed

    // Classic BPF, offsets from the IP header. Jumps count instructions to skip.
    // Ancillary data (packet type) sits at a negative offset, wrapped to u32
    constexpr auto kPktType = static_cast<__u32>(SKF_AD_OFF + SKF_AD_PKTTYPE);
    sock_filter code[] = {
        /* 0 */ BPF_STMT(BPF_LD | BPF_B | BPF_ABS, kPktType),
        /* 1 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, PACKET_HOST, 0, 8),  // unicast to us
        /* 2 */ BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 9),                   // IP protocol
        /* 3 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_UDP, 0, 6),
        /* 4 */ BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 6),                   // flags + fragment offset
        /* 5 */ BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, 0x1fff, 4, 0),      // not a later fragment
        /* 6 */ BPF_STMT(BPF_LDX | BPF_B | BPF_MSH, 0),                  // X = IP header length
        /* 7 */ BPF_STMT(BPF_LD | BPF_H | BPF_IND, 2),                   // UDP dst port
        /* 8 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, port, 0, 1),
        /* 9 */ BPF_STMT(BPF_RET | BPF_K, 0x40000),                      // accept (max bytes kept)
        /* 10 */ BPF_STMT(BPF_RET | BPF_K, 0),                           // drop
    };
    sock_fprog prog{};
    prog.len = sizeof code / sizeof code[0];
    prog.filter = code;
    if (::setsockopt(fd(), SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof prog) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt SO_ATTACH_FILTER");
    }

    const unsigned ifindex = ::if_nametoindex(interface.c_str());
    if (ifindex == 0) {
        const int err = errno;  // before building the message: allocation may change errno
        throw std::system_error(err, std::generic_category(), "interface '" + interface + "'");
    }

    sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_IP);  // IPv4 only (no ARP, IPv6)
    addr.sll_ifindex = static_cast<int>(ifindex);
    if (::bind(fd(), reinterpret_cast<const sockaddr*>(&addr), sizeof addr) < 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(), "bind AF_PACKET to " + interface);
    }
}
