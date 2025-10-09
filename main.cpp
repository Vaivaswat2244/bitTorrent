#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include <iterator> // Required for the new file reading method

#include "bencode_parser.hpp"
#include "tracker.hpp"
#include "bittorrent/logging.hpp"
#include <algorithm>
#include <openssl/sha.h>

static btt::LogLevel parse_loglevel(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "trace") return btt::LogLevel::TRACE;
    if (lower == "debug") return btt::LogLevel::DEBUG;
    if (lower == "info")  return btt::LogLevel::INFO;
    if (lower == "warn" || lower == "warning") return btt::LogLevel::WARN;
    if (lower == "error") return btt::LogLevel::ERROR;
    return btt::LogLevel::INFO;
}

int main(int argc, char* argv[]) {
    // Default logging settings
    btt::LogLevel log_level = btt::LogLevel::INFO;
    std::string log_file;
    std::string torrent_filename;
    bool test_logging = false;

    // Simple flag parsing: accepts --log-level=LEVEL, --log-file=PATH, --test-logging, and a torrent file path
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--log-level=", 0) == 0) {
            log_level = parse_loglevel(a.substr(12));
        } else if (a.rfind("--log-file=", 0) == 0) {
            log_file = a.substr(11);
        } else if (a == "--test-logging") {
            test_logging = true;
        } else if (a == "--help" || a == "-h") {
            std::cout << "Usage: " << argv[0] << " [--log-level=trace|debug|info|warn|error] [--log-file=path] [--test-logging] <torrent_file>\n";
            return 0;
        } else if (a.size() > 0 && a[0] != '-') {
            torrent_filename = a;
        }
    }

    // initialize logging
    btt::init_logging(log_level, log_file);
    LOG_INFO("Starting TinyTorrent (log_level set)");

    if (test_logging) {
        // Exercise the logger so you can see output for all levels
        LOG_TRACE("This is a TRACE message");
        LOG_DEBUG("This is a DEBUG message");
        LOG_INFO("This is an INFO message");
        LOG_WARN("This is a WARN message");
        LOG_ERROR("This is an ERROR message");
        return 0;
    }

    if (torrent_filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " <torrent_file> (or use --help)" << std::endl;
        return 1;
    }
    
    // --- NEW, MOST ROBUST FILE READING METHOD ---
    std::ifstream torrent_file(torrent_filename, std::ios::binary);
    if (!torrent_file.is_open()) {
        std::cerr << "Error: Could not open file " << torrent_filename << std::endl;
        return 1;
    }
    
    // Use stream iterators to read the entire file into a string.
    // This is the most robust way to "slurp" a file.
    std::string file_contents((std::istreambuf_iterator<char>(torrent_file)),
                               std::istreambuf_iterator<char>());
    torrent_file.close();
    
    LOG_INFO("Read " << file_contents.length() << " bytes from torrent file.");
    // --- END OF NEW FILE READING ---

    try {
        // --- Calculate Info Hash ---
        size_t info_key_pos = file_contents.find("4:info");
        if (info_key_pos == std::string::npos) {
            throw std::runtime_error("'info' key not found in torrent file");
        }
        
        size_t info_start_pos = info_key_pos + 6;
        std::string_view info_view(file_contents.data() + info_start_pos, file_contents.length() - info_start_pos);

        if (info_view.front() != 'd') {
            throw std::runtime_error("Info dictionary does not start with 'd'");
        }

        int nesting_level = 0;
        size_t info_end_pos = 0;
        for (char c : info_view) {
            if (c == 'd' || c == 'l') {
                nesting_level++;
            } else if (c == 'e') {
                nesting_level--;
            }
            info_end_pos++;
            if (nesting_level == 0) {
                break;
            }
        }
        
        if (nesting_level != 0) {
            throw std::runtime_error("Malformed info dictionary in torrent file");
        }

        std::string_view info_bencoded = info_view.substr(0, info_end_pos);

        unsigned char info_hash_raw[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(info_bencoded.data()), info_bencoded.length(), info_hash_raw);
        std::string info_hash(reinterpret_cast<char*>(info_hash_raw), SHA_DIGEST_LENGTH);
        
        // --- Parse metadata from the torrent file ---
        std::string_view file_view_for_parser = file_contents;
        BencodeValue parsed_data = parse_bencoded_value(file_view_for_parser);
        BencodeDict& root_dict = std::get<BencodeDict>(parsed_data.data);
        
        std::string announce_url = std::get<std::string>(root_dict.at("announce").data);

    LOG_INFO("Tracker URL: " << announce_url);
        
        // --- Contact the tracker to get peers ---
    LOG_INFO("Contacting tracker...");
    std::vector<Peer> peers = get_peers_from_tracker(announce_url, info_hash);

    LOG_INFO("Received " << peers.size() << " peers:");
        int count = 0;
        for (const auto& peer : peers) {
            LOG_INFO("  - " << peer.ip << ":" << peer.port);
            if (++count >= 10) break;
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Error: " << e.what());
        return 1;
    }

    return 0;
}