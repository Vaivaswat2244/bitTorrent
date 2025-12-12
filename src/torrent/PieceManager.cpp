#include "bittorrent/torrent/PieceManager.hpp"
#include "bittorrent/peer/PeerWire.hpp"
#include "bittorrent/logging.hpp"
#include <openssl/sha.h>
#include <fstream>
#include <vector>
#include <cstring>

PieceManager::PieceManager(const TorrentFile& torrent, const std::string& output_path)
    : torrent_(torrent), output_path_(output_path) {}

static bool sha1_matches(const std::vector<uint8_t>& data, const std::vector<uint8_t>& expected) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(data.data(), data.size(), hash);
    if (expected.size() != SHA_DIGEST_LENGTH) return false;
    return std::memcmp(hash, expected.data(), SHA_DIGEST_LENGTH) == 0;
}

bool PieceManager::download_all_sequential(PeerWire& wire, const std::vector<uint8_t>& peer_id) {
    size_t total_pieces = torrent_.get_total_pieces();
    uint32_t piece_length = torrent_.get_piece_length();

    // Open output file for writing entire content (single-file torrents only for now)
    std::ofstream out(output_path_, std::ios::binary | std::ios::out);
    if (!out.is_open()) {
        LOG_ERROR("Could not open output file: " << output_path_);
        return false;
    }

    for (size_t idx = 0; idx < total_pieces; ++idx) {
        uint32_t this_piece_length = piece_length;
        uint64_t offset = static_cast<uint64_t>(idx) * piece_length;
        // Last piece may be smaller
        if (idx == total_pieces - 1) {
            uint64_t total_len = torrent_.get_total_length();
            uint64_t rem = total_len - offset;
            this_piece_length = static_cast<uint32_t>(rem);
        }

        LOG_INFO("Requesting piece " << idx << " length=" << this_piece_length);
        // We'll request the entire piece in one request for simplicity
        if (!wire.send_request(static_cast<uint32_t>(idx), 0, this_piece_length)) {
            LOG_ERROR("send_request failed for piece " << idx);
            out.close();
            return false;
        }

        uint32_t rindex=0, rbegin=0;
        std::vector<uint8_t> block;
        if (!wire.receive_piece(rindex, rbegin, block)) {
            LOG_ERROR("Failed to receive piece " << idx);
            out.close();
            return false;
        }
        if (rindex != idx || rbegin != 0) {
            LOG_ERROR("Received unexpected piece index/begin: " << rindex << "," << rbegin);
            out.close();
            return false;
        }

        // Verify SHA-1
        std::vector<uint8_t> expected = torrent_.get_piece_hash(idx);
        if (!sha1_matches(block, expected)) {
            LOG_ERROR("Piece " << idx << " failed SHA-1 verification");
            out.close();
            return false;
        }

        // Write at offset
        out.seekp(static_cast<std::streamoff>(offset));
        out.write(reinterpret_cast<const char*>(block.data()), block.size());
        out.flush();
        LOG_INFO("Wrote piece " << idx << " to " << output_path_);
    }

    out.close();
    LOG_INFO("All pieces downloaded and verified");
    return true;
}
