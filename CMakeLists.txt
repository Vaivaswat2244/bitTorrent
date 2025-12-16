cmake_minimum_required(VERSION 3.10)

project(TinyTorrent CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(tinytorrent main.cpp 
    src/bencode/bencode_parser.cpp 
    src/tracker/tracker.cpp 
    src/logging.cpp 
    src/torrent/TorrentFile.cpp
    src/peer/TCPClient.cpp
    src/peer/PeerWire.cpp
    src/peer/PeerDownloader.cpp
    src/torrent/PieceManager.cpp
)
target_include_directories(tinytorrent PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bittorrent
    ${CMAKE_SOURCE_DIR}/include/bittorrent/bencode
    ${CMAKE_SOURCE_DIR}/include/bittorrent/torrent
    ${CMAKE_SOURCE_DIR}/include/bittorrent/tracker
)
target_link_libraries(tinytorrent PRIVATE
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
)