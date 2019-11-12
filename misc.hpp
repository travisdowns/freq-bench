#ifndef MISC_H_
#define MISC_H_

#include <algorithm>
#include <memory>
#include <cstdio>
#include <cctype>
#include <cassert>
#include <vector>
#include <ostream>
#include <functional>
#include <iterator>

/* miscellaneous stuff that's useful for modern (ha?) C++ */

/**
 * Sometimes it's useful to have a functor object rather than a template
 * function since this can be passed to a function which may apply it to d
 * different object types.
 */
struct min_functor {
    template <typename T>
    const T& operator()(const T& l, const T& r) const { return std::min(l, r); }
};

struct max_functor {
    template <typename T>
    const T& operator()(const T& l, const T& r) const { return std::max(l, r); }
};

/*
 * Given a printf-style format and args, return the formatted string as a std::string.
 *
 * See https://stackoverflow.com/a/26221725/149138.
 */
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args) {
    size_t size = std::snprintf( nullptr, 0, format.c_str(), args ..., 0 ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ..., 0 ); // 0 is a dirty hack to avoid warning about no args
    return buf.get();
}

/*
 * Split a string delimited by sep.
 *
 * See https://stackoverflow.com/a/7408245/149138
 */
static inline std::vector<std::string> split(const std::string &text, const std::string &sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + sep.length();
  }
  tokens.push_back(text.substr(start));
  return tokens;
}

static inline std::string string_toupper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

/**
 * Holds a pointer to the start of an an array and its size, so
 * it can implement the begin()/end() contract that various
 * algorithms want.
 */
template <typename T>
struct array_holder {
    T* p;
    size_t size_;

    array_holder(T* p, size_t size) : p{p}, size_{size} {}

    const T& operator[](size_t i) const {
        assert(i < size_);
        return p[i];
    }

    size_t size() const { return size_; }
};

// https://stackoverflow.com/a/4415646/149138
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    bool first = true;
    for (auto&& val : vec) {
        if (!first) os << ",";
        os << val;
        first = false;
    }
    os << "]";
    return os;
}

void clflush(const void *storage, size_t size);

#endif
