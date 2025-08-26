#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <string_view>

// Forward declarations for recursive structures
struct BencodeValue;
using BencodeList = std::vector<BencodeValue>;
using BencodeDict = std::map<std::string, BencodeValue>;

// std::variant allows us to store different types in one variable.
// A BencodeValue can be a string, an integer, a list, or a dictionary.
struct BencodeValue {
    std::variant<std::string, long long, BencodeList, BencodeDict> data;
};

// This function will be the entry point to our parser.
// It takes a string_view (an efficient, non-owning view of a string)
// of the bencoded data and returns the parsed structure.
BencodeValue parse_bencoded_value(std::string_view& bencoded_data);