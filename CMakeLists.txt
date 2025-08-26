cmake_minimum_required(VERSION 3.10)

project(TinyTorrent CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(tinytorrent main.cpp bencode_parser.cpp tracker.cpp)
target_link_libraries(tinytorrent PRIVATE
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
)