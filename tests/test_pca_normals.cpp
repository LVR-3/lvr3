#include <gtest/gtest.h>

#include "lvr2/geometry/BaseVector.hpp"
#include "lvr2/reconstruction/AdaptiveKSearchSurface.hpp"
#include "lvr2/types/PointBuffer.hpp"

#include <boost/shared_array.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace
{

using Vec = lvr2::BaseVector<float>;

constexpr float kPi = 3.14159265358979323846f;

float length(const Vec& vector)
{
    return std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

Vec normalized(Vec vector)
{
    const float len = length(vector);
    return Vec(vector.x / len, vector.y / len, vector.z / len);
}

float absUnitDot(const Vec& lhs, const Vec& rhs)
{
    const Vec lhsUnit = normalized(lhs);
    const Vec rhsUnit = normalized(rhs);
    return std::abs(lhsUnit.x * rhsUnit.x + lhsUnit.y * rhsUnit.y + lhsUnit.z * rhsUnit.z);
}

float minCosineForDegrees(float degrees)
{
    return std::cos(degrees * kPi / 180.0f);
}

lvr2::PointBufferPtr makePointBuffer(const std::vector<Vec>& points)
{
    boost::shared_array<float> raw(new float[points.size() * 3]);
    for(std::size_t i = 0; i < points.size(); ++i)
    {
        raw[i * 3 + 0] = points[i].x;
        raw[i * 3 + 1] = points[i].y;
        raw[i * 3 + 2] = points[i].z;
    }

    return std::make_shared<lvr2::PointBuffer>(raw, points.size());
}

void setNormals(lvr2::PointBufferPtr buffer, const std::vector<Vec>& normals)
{
    boost::shared_array<float> raw(new float[normals.size() * 3]);
    for(std::size_t i = 0; i < normals.size(); ++i)
    {
        raw[i * 3 + 0] = normals[i].x;
        raw[i * 3 + 1] = normals[i].y;
        raw[i * 3 + 2] = normals[i].z;
    }
    buffer->setNormalArray(raw, normals.size());
}

std::string writePoseFile(const std::string& name, const std::vector<Vec>& poses)
{
    const std::string path = ::testing::TempDir() + name;
    std::ofstream out(path);
    for(const Vec& pose : poses)
    {
        out << pose.x << ' ' << pose.y << ' ' << pose.z << '\n';
    }
    return path;
}

lvr2::PointBufferPtr estimateNormals(
    const std::vector<Vec>& points,
    int method,
    std::uint32_t seed = 0u,
    int neighbors = 8,
    const std::string& poseFile = ""
)
{
    auto buffer = makePointBuffer(points);
    lvr2::AdaptiveKSearchSurface<Vec> surface(
        buffer,
        "lvr2",
        neighbors,
        0,
        neighbors,
        method,
        poseFile,
        seed
    );
    surface.calculateSurfaceNormals();
    return buffer;
}

lvr2::PointBufferPtr estimatePcaNormals(const std::vector<Vec>& points)
{
    return estimateNormals(points, 0);
}

Vec normalAt(const boost::shared_array<float>& normals, std::size_t index)
{
    return Vec(normals[index * 3 + 0], normals[index * 3 + 1], normals[index * 3 + 2]);
}

void expectFiniteUnitNormal(const Vec& normal)
{
    EXPECT_TRUE(std::isfinite(normal.x));
    EXPECT_TRUE(std::isfinite(normal.y));
    EXPECT_TRUE(std::isfinite(normal.z));
    EXPECT_NEAR(length(normal), 1.0f, 1.0e-4f);
}

std::vector<Vec> makeVerticalPlaneFixture()
{
    std::vector<Vec> points;
    for(int y = -2; y <= 2; ++y)
    {
        for(int z = -2; z <= 2; ++z)
        {
            points.emplace_back(2.0f, static_cast<float>(y), static_cast<float>(z));
        }
    }
    return points;
}

std::vector<Vec> makeNoisyTiltedPlaneFixture()
{
    const Vec normal = normalized(Vec(1.0f, 1.0f, 1.0f));
    std::vector<Vec> points;

    for(int y = -3; y <= 3; ++y)
    {
        for(int z = -3; z <= 3; ++z)
        {
            const float fy = static_cast<float>(y) * 0.25f;
            const float fz = static_cast<float>(z) * 0.25f;
            const float x = 1.0f - fy - fz;
            const float noise = static_cast<float>(((y + 3) * 5 + (z + 3) * 3) % 5 - 2) * 0.0025f;
            points.emplace_back(
                x + noise * normal.x,
                fy + noise * normal.y,
                fz + noise * normal.z
            );
        }
    }

    return points;
}

std::vector<Vec> makeSpherePatchFixture()
{
    constexpr float radius = 2.0f;
    std::vector<Vec> points;

    for(int y = -3; y <= 3; ++y)
    {
        for(int z = -3; z <= 3; ++z)
        {
            const float fy = static_cast<float>(y) * 0.1f;
            const float fz = static_cast<float>(z) * 0.1f;
            const float x = std::sqrt(radius * radius - fy * fy - fz * fz);
            points.emplace_back(x, fy, fz);
        }
    }

    return points;
}

constexpr std::size_t kPlaneOutlierInlierCount = 49;

std::vector<Vec> makeHorizontalPlaneWithOutliersFixture()
{
    std::vector<Vec> points;
    for(int x = -3; x <= 3; ++x)
    {
        for(int y = -3; y <= 3; ++y)
        {
            const float fx = static_cast<float>(x) * 0.2f;
            const float fy = static_cast<float>(y) * 0.2f;
            const float noise = static_cast<float>(((x + 3) * 7 + (y + 3) * 11) % 5 - 2) * 0.001f;
            points.emplace_back(fx, fy, noise);
        }
    }

    points.emplace_back(-0.4f, -0.4f, 1.5f);
    points.emplace_back(0.0f, -0.2f, -1.4f);
    points.emplace_back(0.4f, 0.2f, 1.6f);
    points.emplace_back(-0.2f, 0.5f, -1.7f);
    points.emplace_back(0.3f, -0.5f, 1.8f);

    return points;
}

std::vector<Vec> makePoseOrientedVerticalPlaneFixture()
{
    std::vector<Vec> points;
    points.emplace_back(0.0f, -0.5f, -0.5f);
    points.emplace_back(-0.05f, 0.0f, 0.0f);
    for(int y = -2; y <= 2; ++y)
    {
        for(int z = -2; z <= 2; ++z)
        {
            if(y == -1 && z == -1)
            {
                continue;
            }
            points.emplace_back(0.0f, static_cast<float>(y) * 0.25f, static_cast<float>(z) * 0.25f);
        }
    }
    return points;
}

std::vector<Vec> translated(const std::vector<Vec>& points, const Vec& offset)
{
    std::vector<Vec> result;
    result.reserve(points.size());
    for(const Vec& point : points)
    {
        result.emplace_back(point.x + offset.x, point.y + offset.y, point.z + offset.z);
    }
    return result;
}

} // namespace

TEST(PcaNormals, DefaultMethodHandlesVerticalPlanes)
{
    const auto points = makeVerticalPlaneFixture();
    const auto buffer = estimatePcaNormals(points);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    const Vec expected(1.0f, 0.0f, 0.0f);
    for(std::size_t i = 0; i < points.size(); ++i)
    {
        const Vec normal = normalAt(normals, i);
        expectFiniteUnitNormal(normal);
        EXPECT_GT(absUnitDot(normal, expected), minCosineForDegrees(1.0f));
    }
}

TEST(PcaNormals, DefaultMethodUsesAngularToleranceOnNoisyPlane)
{
    const auto points = makeNoisyTiltedPlaneFixture();
    const auto buffer = estimatePcaNormals(points);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    const Vec expected = normalized(Vec(1.0f, 1.0f, 1.0f));
    for(std::size_t i = 0; i < points.size(); ++i)
    {
        const Vec normal = normalAt(normals, i);
        expectFiniteUnitNormal(normal);
        EXPECT_GT(absUnitDot(normal, expected), minCosineForDegrees(3.0f));
    }
}

TEST(PcaNormals, DefaultMethodAlignsSpherePatchNormalsRadially)
{
    const auto points = makeSpherePatchFixture();
    const auto buffer = estimatePcaNormals(points);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    for(std::size_t i = 0; i < points.size(); ++i)
    {
        const Vec normal = normalAt(normals, i);
        expectFiniteUnitNormal(normal);
        EXPECT_GT(absUnitDot(normal, points[i]), minCosineForDegrees(15.0f));
    }
}

TEST(PcaNormals, DefaultMethodKeepsDegenerateNormalsFinite)
{
    const std::vector<Vec> points(8, Vec(1.0f, -2.0f, 0.5f));
    const auto buffer = estimatePcaNormals(points);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    for(std::size_t i = 0; i < points.size(); ++i)
    {
        expectFiniteUnitNormal(normalAt(normals, i));
    }
}

TEST(RansacNormals, MethodHandlesPlaneWithOutliersUsingAngularTolerance)
{
    const auto points = makeHorizontalPlaneWithOutliersFixture();
    const auto buffer = estimateNormals(points, 1, 1234u, 12);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    const Vec expected(0.0f, 0.0f, 1.0f);
    for(std::size_t i = 0; i < kPlaneOutlierInlierCount; ++i)
    {
        const Vec normal = normalAt(normals, i);
        expectFiniteUnitNormal(normal);
        EXPECT_GT(absUnitDot(normal, expected), minCosineForDegrees(5.0f));
    }
}

TEST(RansacNormals, SameSeedIsReproducible)
{
    const auto points = makeHorizontalPlaneWithOutliersFixture();
    const auto first = estimateNormals(points, 1, 77u, 12)->getNormalArray();
    const auto second = estimateNormals(points, 1, 77u, 12)->getNormalArray();
    ASSERT_NE(first.get(), nullptr);
    ASSERT_NE(second.get(), nullptr);

    for(std::size_t i = 0; i < points.size(); ++i)
    {
        const Vec firstNormal = normalAt(first, i);
        const Vec secondNormal = normalAt(second, i);
        expectFiniteUnitNormal(firstNormal);
        expectFiniteUnitNormal(secondNormal);
        EXPECT_FLOAT_EQ(firstNormal.x, secondNormal.x);
        EXPECT_FLOAT_EQ(firstNormal.y, secondNormal.y);
        EXPECT_FLOAT_EQ(firstNormal.z, secondNormal.z);
    }
}

TEST(RansacNormals, DegenerateNeighborhoodsFallBackToFiniteNormals)
{
    const std::vector<Vec> points(8, Vec(1.0f, -2.0f, 0.5f));
    const auto buffer = estimateNormals(points, 1, 99u, 3);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    for(std::size_t i = 0; i < points.size(); ++i)
    {
        expectFiniteUnitNormal(normalAt(normals, i));
    }
}

TEST(NormalOrientation, ScanPoseIdsIndexStoredPosePositions)
{
    const auto points = makePoseOrientedVerticalPlaneFixture();
    const std::string poseFile = writePoseFile(
        "normal_pose_orientation.txt",
        {Vec(100.0f, 0.0f, 0.0f), Vec(5.0f, 0.0f, 0.0f)}
    );

    const auto buffer = estimateNormals(points, 0, 0u, 8, poseFile);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    const Vec expected(1.0f, 0.0f, 0.0f);
    for(std::size_t i = 2; i < points.size(); ++i)
    {
        const Vec normal = normalAt(normals, i);
        expectFiniteUnitNormal(normal);
        EXPECT_GT(normalized(normal).dot(expected), minCosineForDegrees(2.0f));
    }
}

TEST(NormalOrientation, ScanPoseOrientationIsTranslationInvariant)
{
    const Vec offset(2.0f, -3.0f, 1.5f);
    const auto points = translated(makePoseOrientedVerticalPlaneFixture(), offset);
    const std::string poseFile = writePoseFile(
        "normal_pose_orientation_translated.txt",
        translated({Vec(100.0f, 0.0f, 0.0f), Vec(5.0f, 0.0f, 0.0f)}, offset)
    );

    const auto buffer = estimateNormals(points, 0, 0u, 8, poseFile);
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    const Vec expected(1.0f, 0.0f, 0.0f);
    for(std::size_t i = 2; i < points.size(); ++i)
    {
        const Vec normal = normalAt(normals, i);
        expectFiniteUnitNormal(normal);
        EXPECT_GT(normalized(normal).dot(expected), minCosineForDegrees(2.0f));
    }
}

TEST(NormalInterpolation, OrientsNeighborsAndSkipsInvalidNormals)
{
    const std::vector<Vec> points{
        Vec(0.0f, 0.0f, 0.0f),
        Vec(0.1f, 0.0f, 0.0f),
        Vec(-0.1f, 0.0f, 0.0f),
        Vec(0.0f, 0.1f, 0.0f),
        Vec(0.0f, -0.1f, 0.0f)
    };
    auto buffer = makePointBuffer(points);
    setNormals(
        buffer,
        {
            Vec(0.0f, 0.0f, 0.0f),
            Vec(1.0f, 0.0f, 0.0f),
            Vec(-1.0f, 0.0f, 0.0f),
            Vec(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f),
            Vec(-1.0f, 0.0f, 0.0f)
        }
    );

    lvr2::AdaptiveKSearchSurface<Vec> surface(buffer, "lvr2", 4, 5, 4, 0);
    const auto directlyQueried = surface.getInterpolatedNormal(points[0]);
    const Vec directlyInterpolated(directlyQueried.x, directlyQueried.y, directlyQueried.z);
    expectFiniteUnitNormal(directlyInterpolated);
    EXPECT_GT(absUnitDot(directlyInterpolated, Vec(1.0f, 0.0f, 0.0f)), minCosineForDegrees(1.0f));

    surface.interpolateSurfaceNormals();
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    const Vec interpolated = normalAt(normals, 0);
    expectFiniteUnitNormal(interpolated);
    EXPECT_GT(absUnitDot(interpolated, Vec(1.0f, 0.0f, 0.0f)), minCosineForDegrees(1.0f));

    const auto queried = surface.getInterpolatedNormal(points[0]);
    const Vec queriedInterpolated(queried.x, queried.y, queried.z);
    expectFiniteUnitNormal(queriedInterpolated);
    EXPECT_GT(absUnitDot(queriedInterpolated, Vec(1.0f, 0.0f, 0.0f)), minCosineForDegrees(1.0f));
}

TEST(NormalInterpolation, AllInvalidNeighborsFallBackToFiniteUnitNormal)
{
    const std::vector<Vec> points{
        Vec(0.0f, 0.0f, 0.0f),
        Vec(0.1f, 0.0f, 0.0f),
        Vec(0.0f, 0.1f, 0.0f)
    };
    auto buffer = makePointBuffer(points);
    setNormals(
        buffer,
        {
            Vec(0.0f, 0.0f, 0.0f),
            Vec(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f),
            Vec(0.0f, 0.0f, 0.0f)
        }
    );

    lvr2::AdaptiveKSearchSurface<Vec> surface(buffer, "lvr2", 3, 3, 3, 0);
    const auto directlyQueried = surface.getInterpolatedNormal(points[0]);
    expectFiniteUnitNormal(Vec(directlyQueried.x, directlyQueried.y, directlyQueried.z));

    surface.interpolateSurfaceNormals();
    const auto normals = buffer->getNormalArray();
    ASSERT_NE(normals.get(), nullptr);

    for(std::size_t i = 0; i < points.size(); ++i)
    {
        expectFiniteUnitNormal(normalAt(normals, i));
    }

    const auto queried = surface.getInterpolatedNormal(points[0]);
    expectFiniteUnitNormal(Vec(queried.x, queried.y, queried.z));
}
