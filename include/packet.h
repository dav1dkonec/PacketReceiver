//
// Created by David Konečný on 08.11.2026.
//

#ifndef LATENCY_RX_PACKET_H
#define LATENCY_RX_PACKET_H

// ---------------------------------------------------------------------------
// Test packet format, shared by the receiver (Pi) and the sender (Mac).
//
// Valid C and C++: plain struct, <stdint.h>, no namespaces or classes.
// All fields are sent in network byte order (big endian), use
// tp_hton32/64 when filling the packet and tp_ntoh32/64 when reading it.
// ---------------------------------------------------------------------------

#include <assert.h>   // static_assert in C11
#include <stddef.h>   // offsetof
#include <stdint.h>

#define TEST_PACKET_MAGIC 0x4C415431u // "LAT1": identifies packets, version 1

struct test_packet {
    uint32_t magic; // TEST_PACKET_MAGIC
    uint32_t seq; // sequence number, starts at 0, +1 per packet
    uint64_t send_ns; // sender timestamp in ns, informative only
};

// Field order keeps every member naturally aligned, so the layout has no
// padding and is the same on arm64 (Pi, Mac) and x86_64 without #pragma pack.
static_assert(sizeof(struct test_packet) == 16, "test_packet must be 16 bytes");
static_assert(offsetof(struct test_packet, seq) == 4, "unexpected test_packet layout");
static_assert(offsetof(struct test_packet, send_ns) == 8, "unexpected test_packet layout");

// Byte order helpers, identical on Linux and macOS (htobe64 is Linux only).
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static inline uint32_t tp_hton32(uint32_t v) { return __builtin_bswap32(v); }
static inline uint64_t tp_hton64(uint64_t v) { return __builtin_bswap64(v); }
#else
static inline uint32_t tp_hton32(uint32_t v) { return v; }
static inline uint64_t tp_hton64(uint64_t v) { return v; }
#endif

static inline uint32_t tp_ntoh32(uint32_t v) { return tp_hton32(v); }
static inline uint64_t tp_ntoh64(uint64_t v) { return tp_hton64(v); }

#endif //LATENCY_RX_PACKET_H
