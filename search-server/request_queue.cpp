#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) 
    : server(search_server), empty_result(0), time(0) {
}

#include "request_queue.h"


std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto result = server.FindTopDocuments(raw_query, status);
    Add_Query_Result(static_cast<int>(result.size()));
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto result = server.FindTopDocuments(raw_query);
    Add_Query_Result(static_cast<int>(result.size()));
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_result;
}

void RequestQueue::Add_Query_Result(const size_t& result_search_Size) {

    ++time;

    if (time > min_in_day_) {
        while (requests_.size() >= min_in_day_) {
            if (requests_.front().result_search == 0) {
                --empty_result;
            }
            requests_.pop_front();
        }
    }

    requests_.push_back({ static_cast<int>(result_search_Size) });

    if (result_search_Size == 0) {
        ++empty_result;
    }
}