#include "bittorrent/peer/PeerWire.hpp"
#include "bittorrent/peer/TCPClient.hpp"
#include "bittorrent/logging.hpp"
#include <openssl/sha.h>
#include <cstring>

static uint32_t be32(const uint8_t* d) {
    return (static_cast<uint32_t>(d[0]) << 24) |
           (static_cast<uint32_t>(d[1]) << 16) |
           (static_cast<uint32_t>(d[2]) << 8)  |
           (static_cast<uint32_t>(d[3]));
}

static void write_be32(uint8_t* d, uint32_t v) {
    d[0] = (v >> 24) & 0xFF;
    d[1] = (v >> 16) & 0xFF;
    d[2] = (v >> 8) & 0xFF;
    d[3] = v & 0xFF;
}

PeerWire::PeerWire(TCPClient& client) : client_(client) {}

bool PeerWire::send_handshake(const TorrentFile& torrent, const std::vector<uint8_t>& peer_id) {
    const std::string pstr = "BitTorrent protocol";
    uint8_t pstrlen = static_cast<uint8_t>(pstr.size());

    std::vector<uint8_t> buf;
    buf.reserve(49 + pstrlen);
    buf.push_back(pstrlen);
    buf.insert(buf.end(), pstr.begin(), pstr.end());
    // 8 reserved bytes
    for (int i = 0; i < 8; ++i) buf.push_back(0);
    // info_hash 20 bytes
    const auto& ih = torrent.get_info_hash();
    buf.insert(buf.end(), ih.begin(), ih.end());
    // peer id 20 bytes
    if (peer_id.size() != 20) {
        LOG_ERROR("peer_id must be 20 bytes");
        return false;
    }
    buf.insert(buf.end(), peer_id.begin(), peer_id.end());

    return client_.send_all(buf);
}

bool PeerWire::recv_and_verify_handshake(const TorrentFile& torrent, std::vector<uint8_t>& remote_peer_id) {
    // First read pstrlen
    std::vector<uint8_t> pbuf;
    if (!client_.recv_n(1, pbuf)) return false;
    uint8_t pstrlen = pbuf[0];

    std::vector<uint8_t> rest;
    if (!client_.recv_n(static_cast<size_t>(pstrlen) + 8 + 20 + 20, rest)) return false;

    // parse
    // pstr is in rest[0..pstrlen-1], skip
    size_t idx = pstrlen;
    // reserved 8 bytes at rest[idx..idx+7]
    idx += 8;
    // info_hash
    const auto& expected_ih = torrent.get_info_hash();
    if (std::memcmp(rest.data() + idx, expected_ih.data(), 20) != 0) {
        LOG_ERROR("Info hash mismatch in handshake");
        return false;
    }
    idx += 20;
    remote_peer_id.assign(rest.begin() + idx, rest.begin() + idx + 20);
    return true;
}

bool PeerWire::send_request(uint32_t index, uint32_t begin, uint32_t length) {
    uint32_t len = 13; // id + index(4) + begin(4) + length(4)
    std::vector<uint8_t> buf(4 + len);
    write_be32(buf.data(), len);
    buf[4] = 6; // request id
    write_be32(buf.data() + 5, index);
    write_be32(buf.data() + 9, begin);
    write_be32(buf.data() + 13, length);
    return client_.send_all(buf);
}

bool PeerWire::receive_piece(uint32_t& index, uint32_t& begin, std::vector<uint8_t>& block) {
    // read length prefix (4)
    std::vector<uint8_t> lb;
    if (!client_.recv_n(4, lb)) return false;
    uint32_t len = be32(lb.data());
    if (len == 0) return true; // keep-alive (no payload)

    std::vector<uint8_t> payload;
    if (!client_.recv_n(len, payload)) return false;
    uint8_t id = payload[0];
    if (id != 7) {
        LOG_DEBUG("Received non-piece message id=" << static_cast<int>(id));
        return false;
    }

    index = be32(payload.data() + 1);
    begin = be32(payload.data() + 5);
    block.assign(payload.begin() + 9, payload.end());
    return true;
}
