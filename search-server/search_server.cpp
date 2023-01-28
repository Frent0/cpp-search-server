#include "search_server.h"


void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {

        if (document_id < 0) {
            throw std::invalid_argument("ѕопытка добавить документ с отрицательным id!");
        }

        if (count(added_id.begin(), added_id.end(), document_id)) {
            throw std::invalid_argument("ѕопытка добавить документ c id ранее добавленного документа!");
        }

        for (const std::string& word : SplitIntoWords(document)) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Ќаличие недопустимых символов!");
            }
        }

        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        const double TF = 1.0 / static_cast<int>(words.size());
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += TF;
        }
        documents_data[document_id] = { ComputeAverageRating(ratings), status };
        added_id.push_back(document_id);
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    }


    int SearchServer::GetDocumentCount() const {
        return static_cast<int>(documents_data.size());
    }

    int SearchServer::GetDocumentId(int index) const {
        if ((index >= 0) && (index < GetDocumentCount())) {
            return added_id[index];
        }
        throw std::out_of_range("»ндекс переданного документа выходит за пределы допустимого диапазона!");
    }

    std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {

        const Query query_word = ParseQuery(raw_query);
        std::vector<std::string> words;

        for (const std::string& word : query_word.plus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                words.push_back(word);
            }
        }

        for (const std::string& word : query_word.minus_word) {
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

    bool SearchServer::IsValidWord(const std::string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    SearchServer::QueryWord SearchServer::ParseQueryWord(std::string word) const {

        if (word.empty()) {
            throw std::invalid_argument("ѕроблема в отсутствие слов после символа <минус> в поисковом запросе!");
        }

        if ((word[0] == '-' && word[1] == '-') || word == "-") {
            throw std::invalid_argument("ѕроблема поискового запроса с отрицательными словами!");
        }
        if (!IsValidWord(word)) {
            throw std::invalid_argument("ѕроблема с наличие недопустимых символов!");
        }

        bool is_minus_word = false;

        if (word[0] == '-') {
            word = word.substr(1);
            is_minus_word = true;
        }

        return { word,is_minus_word,IsStopWord(word) };
    }

    SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
        Query query_words;
        for (std::string& word : SearchServer::SplitIntoWordsNoStop(text)) {
            QueryWord qyery_word = ParseQueryWord(word);
            if (!qyery_word.stop_word) {
                if (qyery_word.minus_word) {
                    query_words.minus_word.insert(qyery_word.word);
                }
                else {
                    query_words.plus_word.insert(qyery_word.word);
                }
            }
        }
        return query_words;
    }

    double SearchServer::IDF(const std::string& word) const {
        return log(SearchServer::GetDocumentCount() * 1.0 / static_cast<size_t>(word_to_document_freqs_.at(word).size()));
    }

    int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        const int result = std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        return result;
    }
