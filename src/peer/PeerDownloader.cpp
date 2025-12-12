#include "bittorrent/peer/TCPClient.hpp"
#include "bittorrent/peer/PeerWire.hpp"
#include "bittorrent/torrent/PieceManager.hpp"
#include "bittorrent/tracker/tracker.hpp"
#include "bittorrent/torrent/TorrentFile.hpp"
#include "bittorrent/logging.hpp"
#include <random>
#include <vector>
#include <string>

// Helper to generate a 20-byte peer id
static std::vector<uint8_t> generate_peer_id() {
    std::vector<uint8_t> id(20);
    const std::string prefix = "-TTC0001-";
    for (size_t i = 0; i < prefix.size() && i < id.size(); ++i) id[i] = prefix[i];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    for (size_t i = prefix.size(); i < id.size(); ++i) id[i] = static_cast<uint8_t>(dis(gen));
    return id;
}

// Connect to a single peer and download sequentially into output_path
bool download_from_peer(const Peer& peer, const TorrentFile& torrent, const std::string& output_path) {
    TCPClient client;
    if (!client.connect_to(peer.ip, peer.port)) {
        LOG_ERROR("Failed to connect to peer " << peer.ip << ":" << peer.port);
        return false;
    }

    PeerWire wire(client);
    auto peer_id = generate_peer_id();
    if (!wire.send_handshake(torrent, peer_id)) {
        LOG_ERROR("Failed to send handshake");
        return false;
    }

    std::vector<uint8_t> remote_peer_id;
    if (!wire.recv_and_verify_handshake(torrent, remote_peer_id)) {
        LOG_ERROR("Handshake verification failed");
        return false;
    }

    LOG_INFO("Handshake completed with peer "+ peer.ip +":" + std::to_string(peer.port));

    PieceManager pm(torrent, output_path);
    bool ok = pm.download_all_sequential(wire, peer_id);
    client.close();
    return ok;
}
