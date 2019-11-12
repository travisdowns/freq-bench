#include "string-instrument.hpp"

#include <unordered_map>

namespace string_metrics {

struct double_holder : public metric_base<double_holder> {
    double d;

    double_holder(double d = 0) : d{d} {}

    virtual double_holder do_delta(const double_holder& rhs) const override {
        return {rhs.d - d};
    }

    virtual double_holder do_add(const double_holder& rhs) const override {
        return {rhs.d + d};
    }

    virtual bool do_less_than(const double_holder& rhs) const override {
        return d < rhs.d;
    }
};

bool Map::contains(const std::string& name) const {
    return map.find(name) != map.end();
}

void Map::add_internal(const std::string& name, Map::ptr p) {
    map.emplace(name, p);
}

template <typename...> struct WhichType;

Map::ptr Map::get_ptr(const std::string& name) const {
    auto i = map.find(name);
    return i == map.end() ? ptr{} : i->second;
}

const metric* Map::get_raw(const std::string& name) const {
    auto i = map.find(name);
    return i == map.end() ? nullptr : &*(i->second);
}

metric* Map::get_raw(const std::string& name) {
    auto p = const_cast<const Map *>(this)->get_raw(name);
    return const_cast<metric*>(p);
}

double& Map::get_double(const std::string& name) {
    double_holder& holder = get<double_holder>(name);
    return holder.d;
}

const double* Map::try_get_double(const std::string& name) const {
    auto p = try_get<double_holder>(name);
    // printf("%p %f %f", p, p->d, const_cast<Map *>(this)->get_double(name));
    return p ? &(p->d) : (double *)nullptr;
}

void Map::clear() {
    map.clear();
}

Map global_;

}
