#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// Forward declaration for your bencode parser
class BencodeValue;

struct FileInfo {
    std::string path;
    uint64_t length;
    std::vector<std::string> path_components; // For multi-file torrents
};

struct TrackerTier {
    std::vector<std::string> urls;
};

class TorrentFile {
public:
    // Constructor that takes a .torrent file path
    explicit TorrentFile(const std::string& torrent_file_path);
    
    // Constructor that takes raw torrent data
    explicit TorrentFile(const std::vector<uint8_t>& torrent_data);
    
    // Getters for torrent metadata
    const std::string& get_announce() const { return announce_; }
    const std::vector<TrackerTier>& get_announce_list() const { return announce_list_; }
    const std::string& get_name() const { return name_; }
    const std::string& get_comment() const { return comment_; }
    const std::string& get_created_by() const { return created_by_; }
    uint64_t get_creation_date() const { return creation_date_; }
    
    // Piece information
    uint32_t get_piece_length() const { return piece_length_; }
    size_t get_total_pieces() const { return piece_hashes_.size() / 20; } // SHA-1 = 20 bytes
    const std::vector<uint8_t>& get_piece_hashes() const { return piece_hashes_; }
    std::vector<uint8_t> get_piece_hash(size_t piece_index) const;
    
    // File information
    const std::vector<FileInfo>& get_files() const { return files_; }
    uint64_t get_total_length() const { return total_length_; }
    bool is_single_file() const { return files_.size() == 1; }
    
    // Info hash (critical for BitTorrent protocol)
    const std::vector<uint8_t>& get_info_hash() const { return info_hash_; }
    std::string get_info_hash_hex() const;
    
    // Validation
    bool is_valid() const { return is_valid_; }
    const std::string& get_error() const { return error_message_; }

private:
    // Parse the bencode data
    void parse_torrent_data(const std::vector<uint8_t>& data);
    void parse_info_dict(const BencodeValue& info_dict);
    void parse_files(const BencodeValue& info_dict);
    void calculate_info_hash(const std::vector<uint8_t>& original_data);
    
    // Torrent metadata
    std::string announce_;
    std::vector<TrackerTier> announce_list_;
    std::string name_;
    std::string comment_;
    std::string created_by_;
    uint64_t creation_date_;
    
    // Piece information  
    uint32_t piece_length_;
    std::vector<uint8_t> piece_hashes_;
    
    // File information
    std::vector<FileInfo> files_;
    uint64_t total_length_;
    
    // Info hash (SHA-1 of bencoded info dict)
    std::vector<uint8_t> info_hash_;
    
    // Validation
    bool is_valid_;
    std::string error_message_;
};