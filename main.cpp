#include <iostream>

// Include headers from our external libraries to check if they link
#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/opensslv.h> // Correct header for version info
#include <boost/version.hpp>

int main() {
    std::cout << "Hello, TinyTorrent!\n";
    std::cout << "We are ready to start building.\n\n";

    // Check libcurl version
    std::cout << "Using " << curl_version() << std::endl;

    // Check OpenSSL version
    // Use the OPENSSL_VERSION_TEXT macro which is simpler and safer
    std::cout << "Using " << OPENSSL_VERSION_TEXT << std::endl;

    // Check Boost version
    std::cout << "Using Boost "
              << BOOST_VERSION / 100000     << "."  // Major version
              << BOOST_VERSION / 100 % 1000 << "."  // Minor version
              << BOOST_VERSION % 100        << std::endl; // Patch level

    return 0;
}