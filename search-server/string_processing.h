#pragma once

#include <vector>
#include <string>
#include <set>

std::vector<std::string> SplitIntoWords(const std::string& text);

std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

template <typename StringContainer>
std::set<std::string, std::less<>> CheckString(const StringContainer& strings) {
    std::set<std::string, std::less<>> result;

    for (auto value : strings) {
        if (!value.empty()) {
            result.emplace(value);
        }
    }

    return result;
}