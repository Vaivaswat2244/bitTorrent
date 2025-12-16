#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "bittorrent/torrent/TorrentFile.hpp"

class TCPClient;

class PeerWire {
public:
    explicit PeerWire(TCPClient& client);

    // Send BitTorrent handshake. Returns true on success.
    bool send_handshake(const TorrentFile& torrent, const std::vector<uint8_t>& peer_id);

    // Receive handshake and verify info_hash matches. Returns true on success.
    bool recv_and_verify_handshake(const TorrentFile& torrent, std::vector<uint8_t>& remote_peer_id);

    // Send a request message for a block
    bool send_request(uint32_t index, uint32_t begin, uint32_t length);

    // Receive a piece message (blocks), returns true on success and fills index, begin and block
    bool receive_piece(uint32_t& index, uint32_t& begin, std::vector<uint8_t>& block);

private:
    TCPClient& client_;
};
