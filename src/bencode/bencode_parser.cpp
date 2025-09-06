#include "bencode_parser.hpp"
#include <stdexcept>
#include <charconv> // For std::from_chars

// Forward declare the main parsing function, as it's recursive
BencodeValue parse_value(std::string_view& bencoded_data);

BencodeValue parse_string(std::string_view& bencoded_data) {
    size_t colon_pos = bencoded_data.find(':');
    if (colon_pos == std::string_view::npos) {
        throw std::runtime_error("Invalid bencoded string: colon not found");
    }

    long long length;
    auto [ptr, ec] = std::from_chars(bencoded_data.data(), bencoded_data.data() + colon_pos, length);

    if (ec != std::errc()) {
        throw std::runtime_error("Invalid bencoded string: invalid length");
    }

    bencoded_data.remove_prefix(colon_pos + 1);
    std::string value = std::string(bencoded_data.substr(0, length));
    bencoded_data.remove_prefix(length);
    
    return BencodeValue{value};
}

BencodeValue parse_integer(std::string_view& bencoded_data) {
    bencoded_data.remove_prefix(1); // Remove 'i'
    size_t end_pos = bencoded_data.find('e');
    if (end_pos == std::string_view::npos) {
        throw std::runtime_error("Invalid bencoded integer: 'e' not found");
    }

    long long value;
    auto [ptr, ec] = std::from_chars(bencoded_data.data(), bencoded_data.data() + end_pos, value);

    if (ec != std::errc()) {
        throw std::runtime_error("Invalid bencoded integer: invalid value");
    }

    bencoded_data.remove_prefix(end_pos + 1);
    return BencodeValue{value};
}

BencodeValue parse_list(std::string_view& bencoded_data) {
    bencoded_data.remove_prefix(1); // Remove 'l'
    BencodeList list;
    while (!bencoded_data.empty() && bencoded_data.front() != 'e') {
        list.push_back(parse_value(bencoded_data));
    }
    if (bencoded_data.empty()) {
        throw std::runtime_error("Invalid bencoded list: 'e' not found");
    }
    bencoded_data.remove_prefix(1); // Remove 'e'
    return BencodeValue{list};
}

BencodeValue parse_dictionary(std::string_view& bencoded_data) {
    bencoded_data.remove_prefix(1); // Remove 'd'
    BencodeDict dict;
    while (!bencoded_data.empty() && bencoded_data.front() != 'e') {
        BencodeValue key_val = parse_string(bencoded_data);
        std::string key = std::get<std::string>(key_val.data);
        BencodeValue value = parse_value(bencoded_data);
        dict[key] = value;
    }
    if (bencoded_data.empty()) {
        throw std::runtime_error("Invalid bencoded dictionary: 'e' not found");
    }
    bencoded_data.remove_prefix(1); // Remove 'e'
    return BencodeValue{dict};
}

BencodeValue parse_value(std::string_view& bencoded_data) {
    if (std::isdigit(bencoded_data.front())) {
        return parse_string(bencoded_data);
    } else if (bencoded_data.front() == 'i') {
        return parse_integer(bencoded_data);
    } else if (bencoded_data.front() == 'l') {
        return parse_list(bencoded_data);
    } else if (bencoded_data.front() == 'd') {
        return parse_dictionary(bencoded_data);
    } else {
        throw std::runtime_error("Invalid bencoded value");
    }
}

// This is the public entry point function declared in the header.
BencodeValue parse_bencoded_value(std::string_view& bencoded_data) {
    return parse_value(bencoded_data);
}