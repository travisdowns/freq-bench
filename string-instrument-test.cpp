// tests for the stuff in randutil.hpp

#include "string-instrument.hpp"

#include "catch.hpp"

#include <typeinfo>

using namespace string_metrics;

struct int_holder : public metric_base<int_holder> {
    int i;

    int_holder(int i = 0) : i{i} {}

    virtual int_holder do_delta(const int_holder& rhs) const override {
        return {rhs.i - i};
    }

    virtual int_holder do_add(const int_holder& rhs) const override {
        return {rhs.i + i};
    }

    virtual bool do_less_than(const int_holder& rhs) const override {
        return i < rhs.i;
    }
};


TEST_CASE("basic-calls", "[string-instrument]") {
    Map map;

    REQUIRE(map.try_get<int_holder>("key") == nullptr);
    REQUIRE(map.contains("key") == false);

    auto& ih = map.get<int_holder>("key");
    REQUIRE(ih.i == 0);
    map.get<int_holder>("key").i++;
    REQUIRE(ih.i == 1);
}

void build_helper(Map& m) {}

template <typename ...Args>
void build_helper(Map& m, std::string key, int value, Args... args) {
    m.get<int_holder>(key) = value;
    build_helper(m, args...);
}

template <typename ...Args>
Map build_map(Args... args) {
    Map m;
    build_helper(m, args...);
    return m;
}

TEST_CASE("build_map", "[string-instrument]") {
    Map m = build_map("key", 1);

    REQUIRE(m.contains("key"));
    REQUIRE(m.size() == 1);

    m = build_map("foo", 1, "bar", 42);

    REQUIRE(m.contains("foo"));
    REQUIRE(m.contains("bar"));
    REQUIRE(m.size() == 2);
    REQUIRE(m.try_get<int_holder>("foo")->i ==  1);
    REQUIRE(m.try_get<int_holder>("bar")->i == 42);
}

TEST_CASE("delta", "[string-instrument]") {
    Map  left = build_map("shared",  1,  "leftonly",  2);
    Map right = build_map("shared", 10, "rightonly", 20);

    Map result = Map::delta(left, right);

    CHECK  (result.size() == 3);
    REQUIRE(result.get<int_holder>("shared").i    ==  9); // 10 - 1
    REQUIRE(result.get<int_holder>("leftonly").i  == -2); //  0 - 2
    REQUIRE(result.get<int_holder>("rightonly").i == 20); // 20 - 0
}
