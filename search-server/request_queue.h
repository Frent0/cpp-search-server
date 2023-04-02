#pragma once

#include "document.h"
#include "search_server.h"

#include <vector>
#include <string>
#include <deque>

class RequestQueue {
public:

    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:

    struct QueryResult {
        int result_search;
    };

    const SearchServer& server;
    std::deque<QueryResult> requests_;

    const static int min_in_day_ = 1440;

    int empty_result;
    int time;

    void Add_Query_Result(const size_t& result_search_Size);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto result_search = server.FindTopDocuments(raw_query, document_predicate);
    Add_Query_Result(result_search.size());
    return result_search;
}