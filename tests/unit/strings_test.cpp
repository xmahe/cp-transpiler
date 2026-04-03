#include <gtest/gtest.h>

#include "support/strings.h"

namespace {

TEST(StringsTest, TrimRemovesOuterWhitespace) {
    EXPECT_EQ(cplus::support::trim("  hello  "), "hello");
}

TEST(StringsTest, SplitAndJoinRoundTrip) {
    const auto parts = cplus::support::split("alpha,beta,gamma", ',');
    ASSERT_EQ(parts.size(), 3U);
    EXPECT_EQ(parts[0], "alpha");
    EXPECT_EQ(parts[1], "beta");
    EXPECT_EQ(parts[2], "gamma");
    EXPECT_EQ(cplus::support::join(parts, "::"), "alpha::beta::gamma");
}

TEST(StringsTest, CaseAndPrefixHelpersWork) {
    EXPECT_TRUE(cplus::support::starts_with("Board___Thermometer", "Board"));
    EXPECT_TRUE(cplus::support::ends_with("Board___Thermometer", "Thermometer"));
    EXPECT_EQ(cplus::support::to_lower("Maybe<U32>"), "maybe<u32>");
}

} // namespace
