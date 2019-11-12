#ifndef STRING_INSTRUMENT_H_
#define STRING_INSTRUMENT_H_

/**
 * Allows you to instrument any algorithm with metrics without passing anything through
 * the call chain.
 *
 * Many metrics are gated behind the SLOW_METRICS define, you should build with that defined
 * to see them (but any timing metrics are bogus in that case).
 */

#include "polymorphic_value.h"

#include <string>
#include <unordered_map>
#include <type_traits>
#include <set>
#include <algorithm>



#ifdef SLOW_METRICS
#define SLOW(...) (__VA_ARGS__)
#define SLOW_ADD(name, increment) string_metrics::global().get_double(name) += increment;
#else
#define SLOW(...)
#define SLOW_ADD(...)
#endif


namespace string_metrics {

struct metric {
    using ptr = jbcoe::polymorphic_value<metric>;

    /* calculate rhs - this and return it as a new object */
    virtual ptr delta(const ptr& rhs) const = 0;

    /* calculate this + rhs and return it as a new object */
    virtual ptr add(const ptr& rhs) const = 0;

    /* return T{} where T is the most derived type of the metric */
    virtual ptr make_default() const = 0;

    /* operator< - return true if this is less than the given rhs */
    virtual bool less_than(const ptr& rhs) const = 0;

    virtual std::string to_string() const { return "N/A"; }

    virtual ~metric() {}
};

inline bool operator<(const metric::ptr& left, const metric::ptr& right) {
    return left->less_than(right);
}

/** CRTP helper to implement make_default */
template <typename T>
struct metric_base : metric {
    virtual metric::ptr make_default() const override {
        return jbcoe::make_polymorphic_value<metric, T>();
    }

    virtual ptr delta(const ptr& rhs) const override {
        auto p = dynamic_cast<const T *>(&*rhs);
        assert(p);
        T t = do_delta(*p);
        return make(t);
    }

    virtual ptr add(const ptr& rhs) const override {
        auto p = dynamic_cast<const T *>(&*rhs);
        assert(p);
        T t = do_add(*p);
        return make(t);
    }

    virtual T do_delta(const T& rhs) const = 0;
    virtual T do_add(const T& rhs) const = 0;

    virtual bool less_than(const ptr& rhs) const override {
        auto p = dynamic_cast<const T *>(&*rhs);
        assert(p);
        return do_less_than(*p);
    }

    virtual bool do_less_than(const T& rhs) const = 0;

    template <typename ...Args>
    static ptr make(Args&&... args) {
        return jbcoe::make_polymorphic_value<metric, T>(std::forward<Args>(args)...);
    }
};


class Map {
    using      ptr = metric::ptr;
    using key_type = std::string;
    using sm_type  = std::unordered_map<key_type, ptr>;

    sm_type map;

    ptr get_ptr(const std::string& name) const;
    const metric* get_raw(const std::string& name) const;
          metric* get_raw(const std::string& name);
    void add_internal(const std::string& name, ptr p);

public:

    /**
     * Return a reference to a double metric with the given name.
     * If the value doesn't exist, it is created and default initailized to zero.
     */
    double& get_double(const std::string& name);

    const double* try_get_double(const std::string& name) const;

    template <typename T>
    T& get(const std::string& name) {
        static_assert(std::is_base_of<metric, T>::value, "type is not a metric");
        auto entry = get_raw(name);
        if (!entry) {
            auto new_entry = jbcoe::make_polymorphic_value<metric, T>();
            // printf("adding new %s at %p\n", name.c_str(), entry);
            add_internal(name, new_entry);
            entry = get_raw(name);
        }
        return *dynamic_cast<T*>(entry);
    }

    template <typename T>
    const T* try_get(const std::string& name) const {
        auto m = get_raw(name);
        return dynamic_cast<const T*>(m);
    }

    /** true iff the element exists */
    bool contains(const std::string& name) const;

    /** number of instrumented objects in the map */
    size_t size() const { return map.size(); }

    /**
     * Clears the entire map.
     */
    void clear();

    Map& operator+=(const Map& rhs) {
        Map sum = apply(*this, rhs, [](auto& l, auto& r){ return l->add(r); });
        *this = sum;
        return *this;
    }

    template <typename F>
    static Map apply(const Map& left, const Map& right, F f) {
        Map ret;
        auto allkeys = left.keys(), rightkeys = right.keys();
        allkeys.insert(rightkeys.begin(), rightkeys.end());

        for (auto& k : allkeys) {
            auto l =  left.get_ptr(k);
            auto r = right.get_ptr(k);
            assert(l || r); // doesn't make sense for them both to be null
            ptr p;
            if (l && r) {
                // key exists in both maps
                assert(typeid(*l) == typeid(*r));
                p = f(l, r);
            } else {
                auto existing = l ? l : r;
                auto def = existing->make_default();
                p = l ? f(l, def) : f(def, r);
            }
            ret.map.insert({k, p});
        }
        return ret;
    }

    /** calculates right - left for all elements with the same name */
    static Map delta(const Map& left, const Map& right) {
        return apply(left, right, [](auto& l, auto& r){ return l->delta(r); });
    }

    /**
     * Return a new map which comtains the minimum value for each metric.
     */
    static Map min(const Map& left, const Map& right) {
        return apply(left, right, [](auto& l, auto& r){ return std::min(l, r); });
    }

    static Map max(const Map& left, const Map& right) {
        return apply(left, right, [](auto& l, auto& r){ return std::max(l, r); });
    }

private:

    std::set<key_type> keys() const {
        std::set<key_type> keys;
        std::transform(map.cbegin(), map.cend(), std::inserter(keys, keys.end()),
                [](auto pair){ return pair.first; });
        return keys;
    }
};

extern Map global_;

/** return the global string map */
inline Map& global() {
    return global_;
}


}

#endif // STRING_INSTRUMENT_H_
