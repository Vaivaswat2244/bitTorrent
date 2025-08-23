# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name and language
project(TinyTorrent CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the required libraries on your system
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
find_package(Boost REQUIRED COMPONENTS system)

# Define our executable, named "tinytorrent", from main.cpp
add_executable(tinytorrent main.cpp)

# Link the found libraries to our executable
target_link_libraries(tinytorrent PRIVATE
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
)