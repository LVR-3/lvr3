#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <vector>

namespace lvr2
{
namespace testing
{

struct PointFixture
{
    float x;
    float y;
    float z;

    bool operator==(const PointFixture& rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
};

inline std::vector<PointFixture> makeGridFixture(std::size_t nx, std::size_t ny, std::size_t nz, float spacing = 1.0f)
{
    std::vector<PointFixture> points;
    points.reserve(nx * ny * nz);

    for (std::size_t z = 0; z < nz; ++z)
    {
        for (std::size_t y = 0; y < ny; ++y)
        {
            for (std::size_t x = 0; x < nx; ++x)
            {
                points.push_back({static_cast<float>(x) * spacing,
                                  static_cast<float>(y) * spacing,
                                  static_cast<float>(z) * spacing});
            }
        }
    }

    return points;
}

inline std::vector<PointFixture> makeUnitCubeFixture()
{
    return {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}
    };
}

inline std::vector<PointFixture> makeCliSmokePointFixture()
{
    return {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };
}

inline bool writePtsFixture(std::ostream& out, const std::vector<PointFixture>& points)
{
    if (!out.good())
    {
        return false;
    }

    for (const auto& point : points)
    {
        out << point.x << ' ' << point.y << ' ' << point.z << '\n';
    }

    return static_cast<bool>(out);
}

inline bool writePtsFixture(const std::vector<PointFixture>& points, const std::filesystem::path& path)
{
    std::ofstream out(path);
    return out.is_open() && writePtsFixture(out, points);
}

inline bool writeObjTriangleFixture(std::ostream& out)
{
    if (!out.good())
    {
        return false;
    }

    out << "# lvr2 test triangle fixture\n";
    out << "v 0 0 0\n";
    out << "v 1 0 0\n";
    out << "v 0 1 0\n";
    out << "f 1 2 3\n";
    return static_cast<bool>(out);
}

inline bool writeObjTriangleFixture(const std::filesystem::path& path)
{
    std::ofstream out(path);
    return out.is_open() && writeObjTriangleFixture(out);
}

} // namespace testing
} // namespace lvr2
