#include "read_input_functions.h"

std::string ReadLine() {
    std::string words;
    std::getline(std::cin, words);
    return words;
}

int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}