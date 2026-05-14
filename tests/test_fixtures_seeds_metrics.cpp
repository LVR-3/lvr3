#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "support/MetricsJsonWriter.hpp"
#include "support/PointFixtures.hpp"
#include "support/SeededRng.hpp"

TEST(T02Fixtures, SyntheticFixturesAreDeterministic)
{
    const auto unitCubeA = lvr2::testing::makeUnitCubeFixture();
    const auto unitCubeB = lvr2::testing::makeUnitCubeFixture();
    EXPECT_EQ(unitCubeA, unitCubeB);

    const auto gridA = lvr2::testing::makeGridFixture(2, 2, 2, 0.5f);
    const auto gridB = lvr2::testing::makeGridFixture(2, 2, 2, 0.5f);
    EXPECT_EQ(gridA, gridB);
    EXPECT_EQ(gridA.size(), 8u);
}

namespace
{

std::filesystem::path makeUniqueTestTempDir(const char* testTag)
{
    static const auto seed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    static std::atomic<std::size_t> counter{0};

    const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    const char* suite = testInfo ? testInfo->test_suite_name() : "unknown_suite";
    const char* name = testInfo ? testInfo->name() : "unknown_test";

    const auto index = counter.fetch_add(1);
    const std::string suffix = std::string(testTag) + "_" + suite + "_" + name + "_" +
                               std::to_string(seed) + "_" + std::to_string(index);

    return std::filesystem::temp_directory_path() / "lvr2_fixture_metrics" / suffix;
}

} // namespace

TEST(T02Fixtures, GeneratedFixturesCoverIoAndCliSmokeInputs)
{
    const auto tempDir = makeUniqueTestTempDir("fixtures");
    std::error_code ec;
    ASSERT_TRUE(std::filesystem::create_directories(tempDir));

    const auto ptsPath = tempDir / "cli-smoke.pts";
    const auto objPath = tempDir / "triangle.obj";

    ASSERT_TRUE(lvr2::testing::writePtsFixture(lvr2::testing::makeCliSmokePointFixture(), ptsPath));
    ASSERT_TRUE(lvr2::testing::writeObjTriangleFixture(objPath));

    std::ifstream pts(ptsPath);
    std::ifstream obj(objPath);
    ASSERT_TRUE(pts.is_open());
    ASSERT_TRUE(obj.is_open());

    std::ostringstream ptsBuffer;
    std::ostringstream objBuffer;
    ptsBuffer << pts.rdbuf();
    objBuffer << obj.rdbuf();

    EXPECT_NE(ptsBuffer.str().find("0 0 0\n1 0 0\n"), std::string::npos);
    EXPECT_NE(objBuffer.str().find("f 1 2 3"), std::string::npos);

    std::filesystem::remove_all(tempDir, ec);
}


TEST(T02Seeds, RandomPermutationIsSeedControlled)
{
    lvr2::testing::SeededRng seededFirst(42);
    lvr2::testing::SeededRng seededSecond(42);
    lvr2::testing::SeededRng seededDifferent(1337);

    const auto first = lvr2::testing::randomPermutation(10, seededFirst.engine());
    const auto second = lvr2::testing::randomPermutation(10, seededSecond.engine());
    const auto third = lvr2::testing::randomPermutation(10, seededDifferent.engine());

    auto sorted = first;
    std::sort(sorted.begin(), sorted.end());

    EXPECT_EQ(first, second);
    EXPECT_EQ(sorted, std::vector<std::size_t>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    EXPECT_EQ(first.size(), 10u);
    EXPECT_NE(third, first);
}

TEST(T02MetricsJson, WritesReadableMetricsReport)
{
    const auto tempDir = makeUniqueTestTempDir("metrics");
    std::error_code ec;
    ASSERT_TRUE(std::filesystem::create_directories(tempDir));
    const auto path = tempDir / "lvr2_fixture_metrics.json";

    lvr2::testing::MetricsReport report;
    report.scenario = "seeded_shuffle_smoke";
    report.entries = {
        {"point_count", 8.0},
        {"sample_head", 5.0},
        {"entropy", 0.815f}
    };

    ASSERT_TRUE(lvr2::testing::writeMetricsJson(report, path));

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());

    std::ostringstream buffer;
    buffer << in.rdbuf();
    const auto content = buffer.str();

    EXPECT_NE(content.find("\"scenario\": \"seeded_shuffle_smoke\""), std::string::npos);
    EXPECT_NE(content.find("\"metric_count\": 3"), std::string::npos);
    EXPECT_NE(content.find("\"name\": \"point_count\""), std::string::npos);
    EXPECT_NE(content.find("\"value\": 8"), std::string::npos);
    in.close();

    std::filesystem::remove_all(tempDir, ec);
}

TEST(T02MetricsJson, EscapesSpecialJsonCharacters)
{
    std::string value = "line";
    value.push_back('\t');
    value.push_back('r');
    value.push_back('\r');
    value.push_back('\n');
    value.push_back('\b');
    value.push_back('\f');
    value += " / quote\" and slash";
    value.push_back(static_cast<char>(0x1F));

    const auto escaped = lvr2::testing::escapeJson(value);
    EXPECT_NE(escaped.find("\\t"), std::string::npos);
    EXPECT_NE(escaped.find("\\r"), std::string::npos);
    EXPECT_NE(escaped.find("\\n"), std::string::npos);
    EXPECT_NE(escaped.find("\\b"), std::string::npos);
    EXPECT_NE(escaped.find("\\f"), std::string::npos);
    EXPECT_NE(escaped.find("\\\""), std::string::npos);
    EXPECT_NE(escaped.find("\\/"), std::string::npos);
    EXPECT_NE(escaped.find("\\u001F"), std::string::npos);
}
