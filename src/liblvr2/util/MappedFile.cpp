#include "lvr2/util/MappedFile.hpp"

#include <cerrno>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#error "lvr2::util::MappedFile currently targets POSIX platforms used by ROS 2 Lyrical/Ubuntu 26.04"
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace lvr2::util
{

MappedFile::MappedFile() noexcept = default;

MappedFile::~MappedFile()
{
    close();
}

MappedFile::MappedFile(MappedFile&& other) noexcept
{
    move_from(other);
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept
{
    if (this != &other)
    {
        close();
        move_from(other);
    }
    return *this;
}

void MappedFile::open_truncated(const std::filesystem::path& path, std::size_t size)
{
    close();

    if (size > static_cast<std::size_t>(std::numeric_limits<off_t>::max()))
    {
        throw std::length_error("MappedFile size exceeds platform file offset range");
    }

    const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        throw std::system_error(errno, std::generic_category(), "open mapped scratch file");
    }

    m_path = path;
    m_fd = fd;
    m_size = size;

    if (::ftruncate(m_fd, static_cast<off_t>(size)) == -1)
    {
        const int error = errno;
        close();
        throw std::system_error(error, std::generic_category(), "resize mapped scratch file");
    }

    if (size == 0)
    {
        return;
    }

    void* mapped = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if (mapped == MAP_FAILED)
    {
        const int error = errno;
        close();
        throw std::system_error(error, std::generic_category(), "map scratch file");
    }

    m_data = static_cast<std::byte*>(mapped);
}

void MappedFile::close() noexcept
{
    if (m_data != nullptr && m_size > 0)
    {
        ::msync(m_data, m_size, MS_SYNC);
        ::munmap(m_data, m_size);
    }

    if (m_fd != -1)
    {
        ::close(m_fd);
    }

    m_path.clear();
    m_data = nullptr;
    m_size = 0;
    m_fd = -1;
}

bool MappedFile::is_open() const noexcept
{
    return m_fd != -1;
}

std::span<std::byte> MappedFile::bytes() noexcept
{
    return {m_data, m_size};
}

std::span<const std::byte> MappedFile::bytes() const noexcept
{
    return {m_data, m_size};
}

void MappedFile::move_from(MappedFile& other) noexcept
{
    m_path = std::move(other.m_path);
    m_data = other.m_data;
    m_size = other.m_size;
    m_fd = other.m_fd;

    other.m_data = nullptr;
    other.m_size = 0;
    other.m_fd = -1;
    other.m_path.clear();
}

} // namespace lvr2::util
