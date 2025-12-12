#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "bittorrent/torrent/TorrentFile.hpp"

class PeerWire;
class TCPClient;

class PieceManager {
public:
    explicit PieceManager(const TorrentFile& torrent, const std::string& output_path);

    // Download all pieces sequentially using provided peer wire on an already-connected socket.
    // Returns true if all pieces downloaded and verified.
    bool download_all_sequential(PeerWire& wire, const std::vector<uint8_t>& peer_id);

private:
    const TorrentFile& torrent_;
    std::string output_path_;
};
