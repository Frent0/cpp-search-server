#pragma once

#include <vector>
#include <string>
#include <set>

std::vector<std::string> SplitIntoWords(const std::string& text);

template <typename StringContainer>
std::set<std::string> CheckString(const StringContainer& strings) {

    std::set<std::string> result;
    for (const std::string& value : strings) {
        if (!value.empty()) {
            result.insert(value);
        }
    }
    return result;

}
