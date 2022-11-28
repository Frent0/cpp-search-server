#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <map>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string words;
    getline(cin, words);
    return words;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL, IRRELEVANT, BANNED, REMOVED
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(const int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double TF = 1.0 / static_cast<size_t>(words.size());
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += TF;
            document_ratings_and_status[document_id] = { ComputeAverageRating(ratings), status };
        }
    }

    template<typename Values>
    vector<Document> FindTopDocuments(const string& raw_query, Values value) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, value);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-10) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document>FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    int GetDocumentCount() const {
        return static_cast<int>(document_ratings_and_status.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, const int document_id) const {
        const Query query_word = ParseQuery(raw_query);
        vector<string> words;

        for (const string& word : query_word.plus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                words.push_back(word);
            }
        }

        for (const string& word : query_word.minus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id) != 0) {
                words.clear();
                break;
            }
        }
        return { words, document_ratings_and_status.at(document_id).status };
    }

private:

    struct DocumentDate {
        int rating;
        DocumentStatus status;
    };

    int document_count_ = 0;

    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentDate> document_ratings_and_status;

    set<string> stop_words_;

    struct QueryWord {
        string word;
        bool is_minus_wordl;
        bool is_stop_word;
    };

    struct Query {
        set<string> minus_word;
        set<string> plus_word;
    };

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus_word = false;
        if (text[0] == '-') {
            is_minus_word = true;
            text = text.substr(1);
        }
        return { text, is_minus_word, IsStopWord(text) };
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            const QueryWord query = ParseQueryWord(word);
            if (!query.is_stop_word) {
                if (query.is_minus_wordl) {
                    query_words.minus_word.insert(query.word);
                }
                else {
                    query_words.plus_word.insert(query.word);
                }
            }

        }
        return query_words;
    }

    double IDF(const string& word) const {
        return log(document_count_ * 1.0 / static_cast<size_t>(word_to_document_freqs_.at(word).size()));
    }
    template<typename Values>
    vector<Document> FindAllDocuments(const Query& query_words, Values values) const {
        vector<Document> matched_documents;
        map<int, double> result;
        for (const string& word : query_words.plus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) {
                const auto& document_status_and_rating = document_ratings_and_status.at(document_id);
                if (values(document_id, document_status_and_rating.status, document_status_and_rating.rating)) {
                    result[document_id] += TF * IDF(word);
                }
            }
        }
        for (const string& word : query_words.minus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, relevance] : word_to_document_freqs_.at(word)) {
                result.erase(document_id);
            }
        }

        for (const auto& [id, relevance] : result) {
            matched_documents.push_back({ id, relevance, document_ratings_and_status.at(id).rating });
        }

        return matched_documents;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        const int result = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        return result;
    }

};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}