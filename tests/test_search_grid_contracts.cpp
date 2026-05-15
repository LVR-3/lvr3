#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/shared_array.hpp>

#include "lvr2/geometry/BaseVector.hpp"
#include "lvr2/geometry/BoundingBox.hpp"
#include "lvr2/types/PointBuffer.hpp"
#include "lvr2/algorithm/KDTree.hpp"
#include "lvr2/reconstruction/HashGrid.hpp"

namespace
{

using Vec = lvr2::BaseVector<float>;
using SearchTree = lvr2::SearchKDTree<Vec>;

class TestBox
{
public:
    static float m_voxelsize;
    static unsigned int INVALID_INDEX;

    explicit TestBox(Vec center)
        : m_center(center)
    {
        std::fill_n(m_vertices, 8, INVALID_INDEX);
    }

    void setVertex(int index, unsigned int value)
    {
        m_vertices[index] = value;
    }

    unsigned int getVertex(int index) const
    {
        return m_vertices[index];
    }

    void setNeighbor(int, TestBox*) {}

    Vec& getCenter()
    {
        return m_center;
    }

    const Vec& getCenter() const
    {
        return m_center;
    }

private:
    Vec m_center;
    unsigned int m_vertices[8];
};

float TestBox::m_voxelsize = 0.0f;
unsigned int TestBox::INVALID_INDEX = std::numeric_limits<unsigned int>::max();

using Grid = lvr2::HashGrid<Vec, TestBox>;

constexpr float kDistanceTolerance = 1.0e-5f;

lvr2::PointBufferPtr makePointBuffer(const std::vector<Vec>& points)
{
    boost::shared_array<float> raw(new float[std::max<std::size_t>(points.size() * 3, 1)]);
    for(std::size_t i = 0; i < points.size(); ++i)
    {
        raw[i * 3 + 0] = points[i].x;
        raw[i * 3 + 1] = points[i].y;
        raw[i * 3 + 2] = points[i].z;
    }

    return std::make_shared<lvr2::PointBuffer>(raw, points.size());
}

float distance(const Vec& lhs, const Vec& rhs)
{
    const float dx = lhs.x - rhs.x;
    const float dy = lhs.y - rhs.y;
    const float dz = lhs.z - rhs.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::vector<std::pair<float, std::size_t>> bruteForceDistances(
    const std::vector<Vec>& points,
    const Vec& query)
{
    std::vector<std::pair<float, std::size_t>> result;
    result.reserve(points.size());
    for(std::size_t i = 0; i < points.size(); ++i)
    {
        result.emplace_back(distance(points[i], query), i);
    }

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        if(std::abs(lhs.first - rhs.first) > kDistanceTolerance)
        {
            return lhs.first < rhs.first;
        }
        return lhs.second < rhs.second;
    });
    return result;
}

void expectNearestPrefix(
    const std::vector<Vec>& points,
    const Vec& query,
    const std::vector<std::size_t>& indices,
    const std::vector<float>& distances,
    std::size_t expectedCount)
{
    const auto brute = bruteForceDistances(points, query);
    ASSERT_EQ(expectedCount, indices.size());
    ASSERT_EQ(expectedCount, distances.size());

    std::vector<bool> seen(points.size(), false);
    for(std::size_t i = 0; i < indices.size(); ++i)
    {
        ASSERT_LT(indices[i], points.size());
        EXPECT_FALSE(seen[indices[i]]) << "duplicate result index " << indices[i];
        seen[indices[i]] = true;
        EXPECT_NEAR(distance(points[indices[i]], query), distances[i], kDistanceTolerance);
    }

    if(expectedCount == 0)
    {
        return;
    }

    const float cutoff = brute[expectedCount - 1].first;
    std::size_t strictlyCloser = 0;
    for(const auto& candidate : brute)
    {
        if(candidate.first < cutoff - kDistanceTolerance)
        {
            ++strictlyCloser;
        }
    }

    std::size_t returnedStrictlyCloser = 0;
    for(std::size_t index : indices)
    {
        const float d = distance(points[index], query);
        EXPECT_LE(d, cutoff + kDistanceTolerance);
        if(d < cutoff - kDistanceTolerance)
        {
            ++returnedStrictlyCloser;
        }
    }
    EXPECT_EQ(strictlyCloser, returnedStrictlyCloser);
}

struct CellSnapshot
{
    int ix;
    int iy;
    int iz;
    std::vector<float> distances;
};

std::vector<CellSnapshot> snapshotGrid(const Grid& grid)
{
    std::vector<CellSnapshot> snapshot;
    for(const auto& entry : grid.getCells())
    {
        const auto& index = entry.first;
        const auto* cell = entry.second;
        CellSnapshot current{index.x(), index.y(), index.z(), {}};
        current.distances.reserve(8);
        for(int vertex = 0; vertex < 8; ++vertex)
        {
            current.distances.push_back(grid.getQueryPoints()[cell->getVertex(vertex)].m_distance);
        }
        snapshot.push_back(current);
    }

    std::sort(snapshot.begin(), snapshot.end(), [](const CellSnapshot& lhs, const CellSnapshot& rhs) {
        return std::tie(lhs.ix, lhs.iy, lhs.iz) < std::tie(rhs.ix, rhs.iy, rhs.iz);
    });
    return snapshot;
}

void expectSameGridCells(const Grid& expected, const Grid& actual)
{
    const auto expectedSnapshot = snapshotGrid(expected);
    const auto actualSnapshot = snapshotGrid(actual);
    ASSERT_EQ(expectedSnapshot.size(), actualSnapshot.size());
    for(std::size_t i = 0; i < expectedSnapshot.size(); ++i)
    {
        EXPECT_EQ(expectedSnapshot[i].ix, actualSnapshot[i].ix);
        EXPECT_EQ(expectedSnapshot[i].iy, actualSnapshot[i].iy);
        EXPECT_EQ(expectedSnapshot[i].iz, actualSnapshot[i].iz);
        ASSERT_EQ(expectedSnapshot[i].distances.size(), actualSnapshot[i].distances.size());
        for(std::size_t d = 0; d < expectedSnapshot[i].distances.size(); ++d)
        {
            EXPECT_NEAR(expectedSnapshot[i].distances[d], actualSnapshot[i].distances[d], kDistanceTolerance);
        }
    }
}

lvr2::BoundingBox<Vec> testBoundingBox()
{
    return lvr2::BoundingBox<Vec>(Vec(-4.0f, -4.0f, -4.0f), Vec(5.0f, 5.0f, 5.0f));
}

Grid makeGrid()
{
    Grid grid(1.0f, testBoundingBox(), true, false);
    grid.addLatticePoint(-1, 0, 0, 0.5f);
    grid.addLatticePoint(0, 0, 0, 0.5f);
    grid.addLatticePoint(2, -1, 1, 1.25f);
    return grid;
}

} // namespace

TEST(SearchContracts, KNearestMatchesBruteForceReference)
{
    const std::vector<Vec> points = {
        Vec(-8.0f, 0.0f, 0.0f),
        Vec(-2.0f, 1.0f, 0.5f),
        Vec(0.0f, 0.0f, 0.0f),
        Vec(0.25f, -0.5f, 3.0f),
        Vec(3.0f, 7.0f, -1.0f),
        Vec(15.0f, -0.25f, 2.0f),
        Vec(-4.5f, -3.0f, 9.0f),
        Vec(0.0f, 0.0f, 0.0f)
    };
    SearchTree tree(makePointBuffer(points));

    std::vector<std::size_t> indices;
    std::vector<float> distances;
    const Vec query(0.5f, -0.25f, 0.75f);
    const int found = tree.kSearch(query, 5, indices, distances);

    ASSERT_EQ(5, found);
    expectNearestPrefix(points, query, indices, distances, 5);
}

TEST(SearchContracts, RadiusSearchReturnsNearestPointsWithinRadius)
{
    const std::vector<Vec> points = {
        Vec(-3.0f, 0.0f, 0.0f),
        Vec(0.0f, 0.0f, 0.0f),
        Vec(1.0f, 0.0f, 0.0f),
        Vec(2.0f, 0.0f, 0.0f),
        Vec(2.0f, 2.0f, 0.0f),
        Vec(6.0f, 0.0f, 0.0f)
    };
    SearchTree tree(makePointBuffer(points));

    std::vector<std::size_t> indices;
    std::vector<float> distances;
    const Vec query(0.0f, 0.0f, 0.0f);
    const int found = tree.radiusSearch(query, 4, 2.0f, indices, distances);

    ASSERT_EQ(3, found);
    expectNearestPrefix(points, query, indices, distances, 3);
    for(float d : distances)
    {
        EXPECT_LE(d, 2.0f + kDistanceTolerance);
    }
}

TEST(SearchContracts, RadiusSearchCapsResultsAtK)
{
    const std::vector<Vec> points = {
        Vec(0.0f, 0.0f, 0.0f),
        Vec(1.0f, 0.0f, 0.0f),
        Vec(2.0f, 0.0f, 0.0f),
        Vec(3.0f, 0.0f, 0.0f)
    };
    SearchTree tree(makePointBuffer(points));

    std::vector<std::size_t> indices;
    std::vector<float> distances;
    const Vec query(0.0f, 0.0f, 0.0f);
    const int found = tree.radiusSearch(query, 2, 3.0f, indices, distances);

    ASSERT_EQ(2, found);
    expectNearestPrefix(points, query, indices, distances, 2);
    for(float d : distances)
    {
        EXPECT_LE(d, 3.0f + kDistanceTolerance);
    }
}

TEST(SearchContracts, NonPositiveRequestsReturnNoResults)
{
    const std::vector<Vec> points = {Vec(0.0f, 0.0f, 0.0f), Vec(1.0f, 0.0f, 0.0f)};
    SearchTree tree(makePointBuffer(points));

    std::vector<std::size_t> indices = {42};
    std::vector<float> distances = {42.0f};

    EXPECT_EQ(0, tree.kSearch(points.front(), 0, indices, distances));
    EXPECT_TRUE(indices.empty());
    EXPECT_TRUE(distances.empty());

    indices = {42};
    distances = {42.0f};
    EXPECT_EQ(0, tree.radiusSearch(points.front(), -1, 1.0f, indices, distances));
    EXPECT_TRUE(indices.empty());
    EXPECT_TRUE(distances.empty());

    indices = {42};
    distances = {42.0f};
    EXPECT_EQ(0, tree.radiusSearch(points.front(), 1, -1.0f, indices, distances));
    EXPECT_TRUE(indices.empty());
    EXPECT_TRUE(distances.empty());
}

TEST(SearchContracts, NoMatchAndEmptyInputsReturnNoResults)
{
    const std::vector<Vec> points = {Vec(10.0f, 0.0f, 0.0f), Vec(11.0f, 0.0f, 0.0f)};
    SearchTree tree(makePointBuffer(points));

    std::vector<std::size_t> indices = {42};
    std::vector<float> distances = {42.0f};
    EXPECT_EQ(0, tree.radiusSearch(Vec(0.0f, 0.0f, 0.0f), 2, 1.0f, indices, distances));
    EXPECT_TRUE(indices.empty());
    EXPECT_TRUE(distances.empty());

    SearchTree emptyTree(makePointBuffer({}));
    indices = {42};
    distances = {42.0f};
    EXPECT_EQ(0, emptyTree.kSearch(Vec(0.0f, 0.0f, 0.0f), 3, indices, distances));
    EXPECT_TRUE(indices.empty());
    EXPECT_TRUE(distances.empty());
}

TEST(HashGridContracts, PointBufferRoundTripPreservesCellsAndDistances)
{
    Grid grid = makeGrid();

    Grid roundTrip(grid.toPointBuffer(), testBoundingBox(), 1.0f);

    expectSameGridCells(grid, roundTrip);
}

TEST(HashGridContracts, SavedGridCanBeReadBack)
{
    Grid grid = makeGrid();
    const std::string path = ::testing::TempDir() + "hash_grid_round_trip.grid";

    grid.saveGrid(path);
    Grid roundTrip(path, testBoundingBox(), 1.0f);

    expectSameGridCells(grid, roundTrip);
    std::remove(path.c_str());
}
