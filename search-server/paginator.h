#pragma once
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) :
        begin_(begin), end_(end), size_(distance(begin, end))
    {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    Iterator size() const {
        return size_;
    }

    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

template<typename iterators>
class Paginator {
public:

    Paginator(iterators begin, iterators end, size_t page_size) {
        for (size_t i = distance(begin, end); i > 0; ) {
            const size_t add_size_page = std::min(i, page_size);
            const auto next_begin = next(begin, add_size_page);
            result.push_back({ begin, next_begin });
            i -= add_size_page;
            begin = next_begin;
        }
    }

    auto begin() const {
        return result.begin();
    }
    auto end() const {
        return result.end();
    }
    auto size() const {
        return result.size();
    }

private:
    std::vector<IteratorRange<iterators>> result;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}