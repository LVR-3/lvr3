#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

namespace lvr2
{
namespace testing
{

class SeededRng
{
public:
    explicit SeededRng(unsigned int seed)
        : m_engine(seed)
    {
    }

    std::mt19937& engine() noexcept
    {
        return m_engine;
    }

    const std::mt19937& engine() const noexcept
    {
        return m_engine;
    }

private:
    std::mt19937 m_engine;
};

inline std::vector<std::size_t> randomPermutation(std::size_t count, unsigned int seed)
{
    std::mt19937 rng(seed);
    std::vector<std::size_t> indices(count);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);
    return indices;
}

inline std::vector<std::size_t> randomPermutation(std::size_t count, std::mt19937& rng)
{
    std::vector<std::size_t> indices(count);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);
    return indices;
}

} // namespace testing
} // namespace lvr2
