#pragma once

#include "document.h"
#include "string_processing.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <deque>
#include <vector>
#include <cmath>
#include <map>
#include <numeric>
#include <optional>


const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:

	SearchServer() = default;

	inline static constexpr int INVALID_DOCUMENT_ID = -1;

	explicit SearchServer(const std::string& stop_words_text) {
		for (const std::string& word : SplitIntoWords(stop_words_text)) {
			if (!IsValidWord(word)) {
				throw std::invalid_argument("Наличие недопустимых символов!");
			}
			else {
				stop_words_.insert(word);
			}
		}
	}

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);


	void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	int GetDocumentCount() const;

	int GetDocumentId(int index) const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:

	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	std::map<std::string, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_data;

	std::set<std::string> stop_words_;

	std::vector<int> added_id;

	struct Query {
		std::set<std::string> minus_word;
		std::set<std::string> plus_word;
	};

	struct QueryWord {
		std::string word;
		bool minus_word;
		bool stop_word;
	};

	static bool IsValidWord(const std::string& word);

	bool IsStopWord(const std::string& word) const;

	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	QueryWord ParseQueryWord(std::string word) const;

	Query ParseQuery(const std::string& text) const;

	double IDF(const std::string& word) const;

	template<typename RequiredRequest>
	std::vector<Document> FindAllDocuments(const Query& query_words, RequiredRequest values) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
	std::string stop_words_text;
	for (const auto& text : stop_words) {
		stop_words_text += " ";
		stop_words_text += text;
	}
	for (const auto& word : SplitIntoWords(stop_words_text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Наличие недопустимых символов!");
		}
		else {
			stop_words_.insert(word);
		}
	}
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {

	const Query query_words = ParseQuery(raw_query);
	std::vector<Document> DocumentsFound = FindAllDocuments(query_words, document_predicate);

	std::sort(DocumentsFound.begin(), DocumentsFound.end(),
		[](const Document& lhs, const Document& rhs) {
			const double  MINIMUM_COMPARISION = 1e-10;
			if (std::abs(lhs.relevance - rhs.relevance) < MINIMUM_COMPARISION) {
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

template<typename RequiredRequest>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query_words, RequiredRequest values) const {
	std::vector<Document> matched_documents;
	std::map<int, double> result;
	for (const std::string& word : query_words.plus_word) {
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
	for (const std::string& word : query_words.minus_word) {
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