#include "request_queue.h"
    
    RequestQueue::RequestQueue(const SearchServer& search_server) : server(search_server), empty_result(0), time(0) {
    }
    
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        auto result_search = server.FindTopDocuments(raw_query, status);
        Add_Query_Result(result_search.size());
        return result_search;
    }
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        auto result_search = server.FindTopDocuments(raw_query);
        Add_Query_Result(result_search.size());
        return result_search;
    }

    int RequestQueue::GetNoResultRequests() const {
        return empty_result;
    }                                                                                                                                                                     ;

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