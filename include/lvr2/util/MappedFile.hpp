#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <type_traits>

namespace lvr2::util
{

/**
 * Small file-backed byte buffer used for scratch storage that previously used
 * a third-party mapped-file helper. The public contract is C++20 byte/span based and
 * intentionally limited to the truncate-and-read/write mode needed by LVR's
 * temporary reconstruction buffers.
 */
class MappedFile
{
  public:
    MappedFile() noexcept;
    ~MappedFile();

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    void open_truncated(const std::filesystem::path& path, std::size_t size);
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }

    [[nodiscard]] std::span<std::byte> bytes() noexcept;
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept;

    template <typename T>
    [[nodiscard]] T* data_as() noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "MappedFile typed data requires trivially copyable values");
        return reinterpret_cast<T*>(m_data);
    }

    template <typename T>
    [[nodiscard]] const T* data_as() const noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "MappedFile typed data requires trivially copyable values");
        return reinterpret_cast<const T*>(m_data);
    }

    template <typename T>
    [[nodiscard]] std::span<T> span_as() noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "MappedFile typed spans require trivially copyable values");
        return {data_as<T>(), m_size / sizeof(T)};
    }

    template <typename T>
    [[nodiscard]] std::span<const T> span_as() const noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>, "MappedFile typed spans require trivially copyable values");
        return {data_as<T>(), m_size / sizeof(T)};
    }

  private:
    void move_from(MappedFile& other) noexcept;

    std::filesystem::path m_path;
    std::byte* m_data = nullptr;
    std::size_t m_size = 0;
    int m_fd = -1;
};

static_assert(!std::is_copy_constructible_v<MappedFile>);
static_assert(std::is_move_constructible_v<MappedFile>);

} // namespace lvr2::util
