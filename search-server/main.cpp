#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <map>
#include <numeric>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double  MINIMUM_COMPARISION = 1e-10;

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

    Document() = default;

    Document(int id_, double relevance_, int rating_) :
        id(id_), relevance(relevance_), rating(rating_) {}

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    explicit SearchServer(const string& stop_words_text) {
        for (const string& word : SplitIntoWords(stop_words_text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Наличие недопустимых символов!");
            }
            else {
                stop_words_.insert(word);
            }
        }
    }

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) {
        string stop_words_text;
        for (const auto& text : stop_words) {
            stop_words_text += " "s;
            stop_words_text += text;
        }
        for (const auto& word : SplitIntoWords(stop_words_text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Наличие недопустимых символов!");
            }
            else {
                stop_words_.insert(word);
            }
        }
    }


    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

        if (document_id < 0) {
            throw invalid_argument("Попытка добавить документ с отрицательным id!");
        }

        if (count(added_id.begin(), added_id.end(), document_id)) {
            throw invalid_argument("Попытка добавить документ c id ранее добавленного документа!");
        }

        for (const string& word : SplitIntoWords(document)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Наличие недопустимых символов!");
            }
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double TF = 1.0 / static_cast<int>(words.size());
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += TF;
        }
        documents_data[document_id] = { ComputeAverageRating(ratings), status };
        added_id.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        vector<Document> DocumentsFound;
        for (const string& word : SplitIntoWords(raw_query)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("В словах поискового запроса есть недопустимые символы!");
            }
        }

        for (const string& word : SplitIntoWords(raw_query)) {
            if (!CorrectRequest(word)) {
                throw invalid_argument("Проблема поискового запроса с отрицательными словами!");
            }
        }

        const Query query_words = ParseQuery(raw_query);

        DocumentsFound = FindAllDocuments(query_words, document_predicate);

        sort(DocumentsFound.begin(), DocumentsFound.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < MINIMUM_COMPARISION) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (DocumentsFound.size() > MAX_RESULT_DOCUMENT_COUNT) {
            DocumentsFound.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return DocumentsFound;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }


    int GetDocumentCount() const {
        return static_cast<int>(documents_data.size());
    }

    int GetDocumentId(int index) const {
        if ((index >= 0) && (index < GetDocumentCount())) {
            return added_id[index];
        }
        throw out_of_range("Индекс переданного документа выходит за пределы допустимого диапазона!");
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {

        for (const string& word : SplitIntoWords(raw_query)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("В словах поискового запроса есть недопустимые символы!");
            }
        }

        for (const string& word : SplitIntoWords(raw_query)) {
            if (!CorrectRequest(word)) {
                throw invalid_argument("Проблема поискового запроса с отрицательными словами!");
            }
        }

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

        return { words, documents_data.at(document_id).status };
    }

private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_data;

    set<string> stop_words_;

    vector<int> added_id;

    struct Query {
        set<string> minus_word;
        set<string> plus_word;
    };

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    static bool CorrectRequest(const string& word) {
        if (word.empty()) {
            return false;
        }
        if (word == "-"s) {
            return false;
        }
        if (word[0] == '-' && word[1] == '-') {
            return false;
        }
        return true;
    }

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

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                word = word.substr(1);
                query_words.minus_word.insert(word);
            }
            else {
                query_words.plus_word.insert(word);
            }
        }
        return query_words;
    }

    double IDF(const string& word) const {
        return log(GetDocumentCount() * 1.0 / static_cast<size_t>(word_to_document_freqs_.at(word).size()));
    }

    template<typename RequiredRequest>
    vector<Document> FindAllDocuments(const Query& query_words, RequiredRequest values) const {
        vector<Document> matched_documents;
        map<int, double> result;
        for (const string& word : query_words.plus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) {
                const auto& document = documents_data.at(document_id);
                if (values(document_id, document.status, document.rating)) {
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
            matched_documents.push_back({ id, relevance, documents_data.at(id).rating });
        }

        return matched_documents;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        const int result = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        return result;
    }

};
