#include "tracker.hpp"
#include "bencode_parser.hpp" // We need this to parse the response
#include <curl/curl.h>
#include <stdexcept>
#include <sstream>
#include <random> // For generating peer_id
#include <arpa/inet.h> // For ntohs

// A callback function for libcurl to write response data into a std::string
size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper to URL-encode a string
std::string url_encode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl for URL encoding");
    }
    char* output = curl_easy_escape(curl, value.c_str(), value.length());
    if (!output) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to URL-encode string");
    }
    std::string result(output);
    curl_free(output);
    curl_easy_cleanup(curl);
    return result;
}

std::vector<Peer> get_peers_from_tracker(const std::string& tracker_url, const std::string& info_hash) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    // 1. Generate a peer_id
    std::string peer_id = "-TTC0001-"; // TinyTorrentClient version 0.0.1
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 255);
    for (int i = 0; i < 12; ++i) {
        peer_id += static_cast<char>(distrib(gen));
    }

    // 2. Build the tracker URL
    std::stringstream url_stream;
    url_stream << tracker_url
               << "?info_hash=" << url_encode(info_hash)
               << "&peer_id=" << url_encode(peer_id)
               << "&port=6881"
               << "&uploaded=0"
               << "&downloaded=0"
               << "&left=0" // We'll fill this in properly later
               << "&compact=1";
    std::string full_url = url_stream.str();

    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // 3. Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
    }
    curl_easy_cleanup(curl);

    // 4. Parse the Bencoded response
    std::string_view response_view = response_data;
    BencodeValue parsed_response = parse_bencoded_value(response_view);
    BencodeDict& response_dict = std::get<BencodeDict>(parsed_response.data);

    if (response_dict.count("failure reason")) {
        throw std::runtime_error("Tracker error: " + std::get<std::string>(response_dict.at("failure reason").data));
    }

    std::string peers_str = std::get<std::string>(response_dict.at("peers").data);
    
    // 5. Decode the compact peer list
    std::vector<Peer> peers;
    for (size_t i = 0; i < peers_str.length(); i += 6) {
        if (i + 6 > peers_str.length()) break;
        
        std::string peer_chunk = peers_str.substr(i, 6);
        Peer p;

        // First 4 bytes are the IP address
        p.ip = std::to_string((unsigned char)peer_chunk[0]) + "." +
               std::to_string((unsigned char)peer_chunk[1]) + "." +
               std::to_string((unsigned char)peer_chunk[2]) + "." +
               std::to_string((unsigned char)peer_chunk[3]);

        // Next 2 bytes are the port in network byte order (big-endian)
        p.port = ntohs(*reinterpret_cast<const uint16_t*>(peer_chunk.data() + 4));

        peers.push_back(p);
    }

    return peers;
}