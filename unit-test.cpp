/*
 * unit-test.cpp
 */

#include "misc.hpp"

#include "catch.hpp"

TEST_CASE( "string_format", "[util]" ) {
    REQUIRE( string_format("foo %d", 42) == "foo 42" );
    REQUIRE( string_format("%s %s", "foo", "bar") == "foo bar" );
}



