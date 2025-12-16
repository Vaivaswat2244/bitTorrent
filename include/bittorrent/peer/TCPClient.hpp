#pragma once

#include <string>
#include <vector>

class TCPClient {
public:
    TCPClient();
    ~TCPClient();

    // Connect to an IPv4 address (dotted) and port. Returns true on success.
    bool connect_to(const std::string& ip, unsigned short port, int timeout_seconds = 5);

    // Send all bytes in buffer. Returns true on success.
    bool send_all(const std::vector<uint8_t>& buf);

    // Receive exactly n bytes. Returns true on success and fills buf.
    bool recv_n(size_t n, std::vector<uint8_t>& buf);

    // Close connection early
    void close();

    bool is_connected() const { return sock_ >= 0; }

private:
    int sock_;
};

