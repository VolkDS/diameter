#ifndef DIAMETER_TESTS_HELPERS_FROM_HEX_H
#define DIAMETER_TESTS_HELPERS_FROM_HEX_H

#include <string>
#include <vector>

inline void load_from_hex(const std::string& s, std::vector<uint8_t>& data)
{
    auto count = s.size() / 2;

    data.clear();
    data.reserve(count);

    for (size_t i = 0; i < s.size(); i += 2) {
        auto substr = s.substr(i, 2);
        auto chr_int = std::stoi(substr, nullptr, 16);
        data.push_back(static_cast<uint8_t>(chr_int));
    }
}

#endif
