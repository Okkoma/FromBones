#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include <climits>

#include "../cpp/ObjectsCore/ShortIntVector2.h"

TEST_CASE("Test Constructor", "[core]") {
    SECTION("s()") {
        ShortIntVector2 s;
        REQUIRE(s.x_ == 0);
        REQUIRE(s.y_ == 0);
    }
    SECTION("s(hash)") {
        ShortIntVector2 s1(SHRT_MIN, SHRT_MIN);
        unsigned hash = s1.ToHash();
        ShortIntVector2 s2(hash);
        REQUIRE(s2.x_ == s1.x_);
        REQUIRE(s2.y_ == s1.y_);
    }
}
