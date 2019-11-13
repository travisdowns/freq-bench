#include "impl-list.hpp"
#include "misc.hpp"
#include "randutil.hpp"

#include <random>
#include <string>
#include <vector>

// this goes last otherwise some operator<< functions are not picked up
#include "catch.hpp"

const char magic = 't';

using velem = std::vector<char>;
using namespace std::string_literals;

const test_description& ref_algo() {
    auto ref_ptr = get_by_name("memcpy_stdlib");
    REQUIRE(ref_ptr);
    return *ref_ptr;
}

TEST_CASE("empty") {
    char buf[] = { magic };

    for (auto algo : get_all()) {
        SECTION(std::string(algo.name)) {
            algo.f({buf, buf, 0});
            CHECK(buf[0] == magic);
        }
    }
}

/** call non-destructive */
void call(const test_description& algo, const char* in, char *out, size_t size) {
    algo.f({out, in, size});
}

TEST_CASE("one") {
    const char ina[] = {'a'}, inB[] = {'B'};
    char out[1]     = { magic };

    for (auto algo : get_all()) {
        if (algo.flags & INCORRECT) {
            // skip incorrect tests
            continue;
        }
        SECTION(std::string(algo.name)) {
            call(algo, ina, out, 1);
            REQUIRE(ina[0] == 'a'); // no overwrite!
            CHECK(out[0] == 'a')    ;

            call(algo, inB, out, 1);
            REQUIRE(inB[0] == 'B'); // no overwrite!
            CHECK(out[0] == 'B');
        }
    }
}

TEST_CASE("sequence") {
    auto& ref = ref_algo();

    for (auto& algo : get_all()) {
        if (algo.flags & INCORRECT) {
            continue;
        }
        SECTION(std::string(algo.name)) {
            for (int s = 1; s < 128; s++) {
                velem input(s);
                velem out_test(s), out_ref(s);

                std::iota(input.begin(), input.end(), (char)0);
                std::fill(out_test.begin(), out_test.end(), (char)-1);
                std::fill(out_ref.begin(), out_ref.end(), (char)-2);

                INFO("input = " << input);

                call(algo, input.data(), out_test.data(), s);
                call(ref, input.data(), out_ref.data(), s);

                REQUIRE(out_test == out_ref);
            }
        }
    }
}

/**
 * Create random small and large arrays (of sizes s and l),
 * filling them with random number generator rng based on
 * distribution dist, and thest that the algo returns the
 * same as the reference.
 */
template <typename R, typename D>
void test_random_with_dist(const test_description& ref, const test_description& algo, size_t s, R& rng, D& dist) {
    auto boundrng = [&](){ return dist(rng); };
    velem input = random_vector<char>(boundrng, s);
    velem out_test(input), out_ref(input);

    algo.f({out_test.data(), input.data(), s});
    ref.f({out_ref.data(), input.data(), s});

    INFO("input = " << input);

    CHECK(out_test == out_ref);
}

TEST_CASE("random") {
    auto& ref = ref_algo();

    pcg32 rng;

    for (auto algo : get_all()) {
        if (algo.flags & INCORRECT) {
            continue;
        }
        SECTION(std::string(algo.name)) {
            for (int s = 1; s < 128; s += 1) {
                auto dist = std::uniform_int_distribution<char>(32, 127); // printable range
                test_random_with_dist(ref, algo, s, rng, dist);
            }
        }
    }
}

TEST_CASE("warn_incorrect") {
    for (auto algo : get_all()) {
        if ((algo.flags & INCORRECT) && algo.name != "dummy"s) {
            WARN("algo " << algo.name << " is flagged as INCORRECT");
        }
    }
}
