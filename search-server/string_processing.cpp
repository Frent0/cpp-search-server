#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char symbol : text) {
        if (symbol == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += symbol;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}