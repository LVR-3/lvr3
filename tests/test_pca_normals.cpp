#include <gtest/gtest.h>

#include "lvr2/geometry/BaseVector.hpp"
#include "lvr2/reconstruction/AdaptiveKSearchSurface.hpp"
#include "lvr2/types/PointBuffer.hpp"

#include <boost/shared_array.hpp>

#include <cmath>
#include <cstddef>
#include <memory>
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

lvr2::PointBufferPtr estimatePcaNormals(const std::vector<Vec>& points)
{
    auto buffer = makePointBuffer(points);
    lvr2::AdaptiveKSearchSurface<Vec> surface(
        buffer,
        "lvr2",
        8,
        0,
        8,
        0,
        ""
    );
    surface.calculateSurfaceNormals();
    return buffer;
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
