#pragma once

#include <string>
#include <vector>

// Represents a single peer (another client) in the swarm
struct Peer {
    std::string ip;
    unsigned short port;
};

// Function to contact the tracker and get a list of peers
// Takes the tracker URL and the 20-byte info_hash
std::vector<Peer> get_peers_from_tracker(const std::string& tracker_url, const std::string& info_hash);