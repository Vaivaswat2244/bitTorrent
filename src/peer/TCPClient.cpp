#include "bittorrent/peer/TCPClient.hpp"
#include "bittorrent/logging.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <cstring>
#include <chrono>

TCPClient::TCPClient() : sock_(-1) {}

TCPClient::~TCPClient() {
    if (sock_ >= 0) ::close(sock_);
}

bool TCPClient::connect_to(const std::string& ip, unsigned short port, int timeout_seconds) {
    if (sock_ >= 0) ::close(sock_);
    sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        LOG_ERROR("socket() failed: " << strerror(errno));
        return false;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        LOG_ERROR("inet_pton failed for " << ip);
        ::close(sock_);
        sock_ = -1;
        return false;
    }

    // Set non-blocking and implement a timeout on connect
    int flags = ::fcntl(sock_, F_GETFL, 0);
    ::fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    int res = ::connect(sock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (res < 0 && errno != EINPROGRESS) {
        LOG_ERROR("connect() failed: " << strerror(errno));
        ::close(sock_);
        sock_ = -1;
        return false;
    }

    if (res == 0) {
        // connected immediately
        ::fcntl(sock_, F_SETFL, flags);
        return true;
    }

    // wait for socket to become writable
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock_, &wfds);
    struct timeval tv;
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;
    res = select(sock_ + 1, nullptr, &wfds, nullptr, &tv);
    if (res <= 0) {
        LOG_ERROR("connect timeout or error: " << strerror(errno));
        ::close(sock_);
        sock_ = -1;
        return false;
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    ::getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0) {
        LOG_ERROR("connect failed after select: " << strerror(so_error));
        ::close(sock_);
        sock_ = -1;
        return false;
    }

    // restore flags
    ::fcntl(sock_, F_SETFL, flags);
    return true;
}

bool TCPClient::send_all(const std::vector<uint8_t>& buf) {
    if (sock_ < 0) return false;
    size_t total = 0;
    while (total < buf.size()) {
        ssize_t sent = ::send(sock_, buf.data() + total, buf.size() - total, 0);
        if (sent <= 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("send failed: " << strerror(errno));
            return false;
        }
        total += static_cast<size_t>(sent);
    }
    return true;
}

bool TCPClient::recv_n(size_t n, std::vector<uint8_t>& buf) {
    buf.clear();
    buf.reserve(n);
    size_t total = 0;
    while (total < n) {
        std::vector<uint8_t> tmp(4096);
        size_t toread = std::min(tmp.size(), n - total);
        ssize_t rec = ::recv(sock_, tmp.data(), toread, 0);
        if (rec <= 0) {
            if (rec == 0) {
                LOG_ERROR("recv returned 0 (peer closed)");
            } else {
                LOG_ERROR("recv failed: " << strerror(errno));
            }
            return false;
        }
        buf.insert(buf.end(), tmp.begin(), tmp.begin() + rec);
        total += static_cast<size_t>(rec);
    }
    return true;
}

void TCPClient::close() {
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

