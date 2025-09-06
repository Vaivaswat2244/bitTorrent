#include "TorrentFile.h"
#include "bencode_parser.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

// This will work on top of the bencode parser.
// From what I saw in parser file, found parsing functions different for dictionaries, list integer and others, this file called the bencode parser and takes the torrent file as input, as the structure of torrent file is [here](https://en.wikipedia.org/wiki/Torrent_file#BitTorrent_v2)
// now we can go ahead and implement the http function to call trackers and get torrent files

TorrentFile::TorrentFile(const std::string& torrent_file_path) 
    : creation_date_(0), piece_length_(0), total_length_(0), is_valid_(false) {
    
    std::ifstream file(torrent_file_path, std::ios::binary);
    if (!file.is_open()) {
        error_message_ = "Cannot open torrent file: " + torrent_file_path;
        return;
    }
    
    // Read entire file into vector
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    file.close();
    
    parse_torrent_data(data);
}

TorrentFile::TorrentFile(const std::vector<uint8_t>& torrent_data)
    : creation_date_(0), piece_length_(0), total_length_(0), is_valid_(false) {
    parse_torrent_data(torrent_data);
}

void TorrentFile::parse_torrent_data(const std::vector<uint8_t>& data) {
    try {
        
        std::string data_str(data.begin(), data.end());
        std::string_view data_view = data_str;
        
        
        BencodeValue parsed_value = parse_bencoded_value(data_view);
        BencodeDict& root_dict = std::get<BencodeDict>(parsed_value.data);
        
        
        auto announce_it = root_dict.find("announce");
        if (announce_it != root_dict.end()) {
            announce_ = std::get<std::string>(announce_it->second.data);
        }
        
       
        auto announce_list_it = root_dict.find("announce-list");
        if (announce_list_it != root_dict.end()) {
            BencodeList& announce_list = std::get<BencodeList>(announce_list_it->second.data);
            for (const auto& tier_value : announce_list) {
                TrackerTier tracker_tier;
                BencodeList& tier_list = std::get<BencodeList>(tier_value.data);
                for (const auto& url_value : tier_list) {
                    tracker_tier.urls.push_back(std::get<std::string>(url_value.data));
                }
                announce_list_.push_back(tracker_tier);
            }
        }
        
        // Parse optional fields
        auto comment_it = root_dict.find("comment");
        if (comment_it != root_dict.end()) {
            comment_ = std::get<std::string>(comment_it->second.data);
        }
        
        auto created_by_it = root_dict.find("created by");
        if (created_by_it != root_dict.end()) {
            created_by_ = std::get<std::string>(created_by_it->second.data);
        }
        
        auto creation_date_it = root_dict.find("creation date");
        if (creation_date_it != root_dict.end()) {
            creation_date_ = std::get<long long>(creation_date_it->second.data);
        }
        
        // Parse info dictionary
        auto info_it = root_dict.find("info");
        if (info_it != root_dict.end()) {
            parse_info_dict(info_it->second);
            calculate_info_hash(data);
        } else {
            throw std::runtime_error("No 'info' dictionary found in torrent");
        }
        
        is_valid_ = true;
        
    } catch (const std::exception& e) {
        error_message_ = "Failed to parse torrent: " + std::string(e.what());
        is_valid_ = false;
    }
}

void TorrentFile::parse_info_dict(const BencodeValue& info_dict) {
    BencodeDict& info_map = std::get<BencodeDict>(info_dict.data);
    
    // Parse name
    auto name_it = info_map.find("name");
    if (name_it != info_map.end()) {
        name_ = std::get<std::string>(name_it->second.data);
    }
    
    // Parse piece length
    auto piece_length_it = info_map.find("piece length");
    if (piece_length_it != info_map.end()) {
        piece_length_ = std::get<long long>(piece_length_it->second.data);
    }
    
    // Parse pieces (SHA-1 hashes concatenated)
    auto pieces_it = info_map.find("pieces");
    if (pieces_it != info_map.end()) {
        std::string pieces_string = std::get<std::string>(pieces_it->second.data);
        piece_hashes_.assign(pieces_string.begin(), pieces_string.end());
    }
    
    // Parse files
    parse_files(info_dict);
}

void TorrentFile::parse_files(const BencodeValue& info_dict) {
    BencodeDict& info_map = std::get<BencodeDict>(info_dict.data);
    
    auto files_it = info_map.find("files");
    if (files_it != info_map.end()) {
        // Multi-file torrent
        BencodeList& files_list = std::get<BencodeList>(files_it->second.data);
        for (const auto& file_entry : files_list) {
            BencodeDict& file_map = std::get<BencodeDict>(file_entry.data);
            
            FileInfo file_info;
            
            auto length_it = file_map.find("length");
            if (length_it != file_map.end()) {
                file_info.length = std::get<long long>(length_it->second.data);
            }
            
            auto path_it = file_map.find("path");
            if (path_it != file_map.end()) {
                BencodeList& path_list = std::get<BencodeList>(path_it->second.data);
                for (const auto& path_component : path_list) {
                    file_info.path_components.push_back(std::get<std::string>(path_component.data));
                }
                
                // Build full path
                std::string full_path;
                for (size_t i = 0; i < file_info.path_components.size(); ++i) {
                    if (i > 0) full_path += "/";
                    full_path += file_info.path_components[i];
                }
                file_info.path = full_path;
            }
            
            files_.push_back(file_info);
            total_length_ += file_info.length;
        }
    } else {
        // Single-file torrent
        auto length_it = info_map.find("length");
        if (length_it != info_map.end()) {
            FileInfo file_info;
            file_info.length = std::get<long long>(length_it->second.data);
            file_info.path = name_;
            file_info.path_components.push_back(name_);
            
            files_.push_back(file_info);
            total_length_ = file_info.length;
        }
    }
}

void TorrentFile::calculate_info_hash(const std::vector<uint8_t>& original_data) {
    // Find the info dictionary in the original bencode data
    // We need to find "4:info" and extract the dictionary that follows
    
    std::string data_str(original_data.begin(), original_data.end());
    size_t info_start = data_str.find("4:info");
    if (info_start == std::string::npos) {
        throw std::runtime_error("Could not find info dictionary in torrent data");
    }
    
    // Move past "4:info" to the start of the dictionary
    info_start += 6;
    
    // Find the matching 'e' for the info dictionary
    // This is tricky because we need to count nested structures
    size_t dict_start = info_start;
    if (data_str[dict_start] != 'd') {
        throw std::runtime_error("Info section is not a dictionary");
    }
    
    // Count braces to find the end of the info dictionary
    int depth = 0;
    size_t info_end = dict_start;
    for (size_t i = dict_start; i < data_str.length(); ++i) {
        if (data_str[i] == 'd' || data_str[i] == 'l') {
            depth++;
        } else if (data_str[i] == 'e') {
            depth--;
            if (depth == 0) {
                info_end = i + 1;
                break;
            }
        }
    }
    
    if (depth != 0) {
        throw std::runtime_error("Could not find end of info dictionary");
    }
    
    // Extract the info dictionary bytes
    std::vector<uint8_t> info_dict_bytes(original_data.begin() + dict_start, 
                                        original_data.begin() + info_end);
    
    // Calculate SHA-1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(info_dict_bytes.data(), info_dict_bytes.size(), hash);
    
    info_hash_.assign(hash, hash + SHA_DIGEST_LENGTH);
}

std::vector<uint8_t> TorrentFile::get_piece_hash(size_t piece_index) const {
    if (piece_index >= get_total_pieces()) {
        return {};
    }
    
    size_t offset = piece_index * 20; // SHA-1 is 20 bytes
    return std::vector<uint8_t>(piece_hashes_.begin() + offset,
                               piece_hashes_.begin() + offset + 20);
}

std::string TorrentFile::get_info_hash_hex() const {
    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (uint8_t byte : info_hash_) {
        ss << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}