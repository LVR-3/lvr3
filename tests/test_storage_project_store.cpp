#include <lvr2/io/scan.hpp>
#include <lvr2/io/storage.hpp>
#include <lvr2/types/PointBuffer.hpp>
#include <lvr2/types/ScanTypes.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{

std::filesystem::path uniqueTempPath(const std::string& suffix)
{
    static std::atomic<unsigned long> counter{0};
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("lvr2-storage-project-" + std::to_string(now) + "-" +
            std::to_string(counter.fetch_add(1)) + suffix);
}

lvr2::PointBufferPtr makePoints()
{
    const std::size_t count = 2;
    lvr2::floatArr points(new float[count * 3]);
    points[0] = 1.0f;
    points[1] = 2.0f;
    points[2] = 3.0f;
    points[3] = 4.0f;
    points[4] = 5.0f;
    points[5] = 6.0f;
    return std::make_shared<lvr2::PointBuffer>(points, count);
}

lvr2::ScanProjectPtr makeProject()
{
    auto project = std::make_shared<lvr2::ScanProject>();
    project->name = "minimal storage project";
    project->coordinateSystem = "right-handed";
    project->unit = "meter";

    auto position = std::make_shared<lvr2::ScanPosition>();
    position->timestamp = 42.0;

    auto lidar = std::make_shared<lvr2::LIDAR>();
    lidar->name = "test lidar";

    auto scan = std::make_shared<lvr2::Scan>();
    scan->startTime = 1.25;
    scan->endTime = 2.5;
    scan->numPoints = 2;
    scan->points = makePoints();

    lidar->scans.push_back(scan);
    position->lidars.push_back(lidar);
    project->positions.push_back(position);
    return project;
}

void expectProjectRoundTrip(const lvr2::ScanProjectPtr& loaded)
{
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->name, "minimal storage project");
    ASSERT_EQ(loaded->positions.size(), 1u);
    ASSERT_EQ(loaded->positions[0]->lidars.size(), 1u);
    ASSERT_EQ(loaded->positions[0]->lidars[0]->scans.size(), 1u);

    const auto& scan = loaded->positions[0]->lidars[0]->scans[0];
    ASSERT_TRUE(scan);
    EXPECT_EQ(scan->numPoints, 2u);
    ASSERT_TRUE(scan->points || scan->loadable());
    if (!scan->points)
    {
        scan->load();
    }
    ASSERT_TRUE(scan->points);
    EXPECT_EQ(scan->points->numPoints(), 2u);

    auto points = scan->points->getPointArray();
    ASSERT_TRUE(points.get() != nullptr);
    EXPECT_NEAR(points[0], 1.0f, 1e-6f);
    EXPECT_NEAR(points[1], 2.0f, 1e-6f);
    EXPECT_NEAR(points[2], 3.0f, 1e-6f);
    EXPECT_NEAR(points[3], 4.0f, 1e-6f);
    EXPECT_NEAR(points[4], 5.0f, 1e-6f);
    EXPECT_NEAR(points[5], 6.0f, 1e-6f);
}

class MemoryBackend final : public lvr2::io::storage::StorageBackend
{
public:
    explicit MemoryBackend(std::string uri)
        : uri_(std::move(uri))
    {
    }

    lvr2::io::storage::BackendInfo info() const override
    {
        return {lvr2::io::storage::StorageKind::named("fake"), uri_, "fake"};
    }

    lvr2::io::storage::Result<bool> exists(const lvr2::io::storage::GroupKey& key) const override
    {
        return hasGroup(key.group);
    }

    lvr2::io::storage::Result<bool> exists(const lvr2::io::storage::DataKey& key) const override
    {
        const std::pair<std::string, std::string> lookup{key.group, key.name};
        return points_.find(lookup) != points_.end() || bytes_.find(lookup) != bytes_.end();
    }

    lvr2::io::storage::Result<std::vector<std::string>> list(const lvr2::io::storage::GroupKey& key) const override
    {
        std::vector<std::string> names;
        for (const auto& entry : points_)
        {
            if (entry.first.first == key.group)
            {
                names.push_back(entry.first.second);
            }
        }
        for (const auto& entry : bytes_)
        {
            if (entry.first.first == key.group)
            {
                names.push_back(entry.first.second);
            }
        }
        return names;
    }

    lvr2::io::storage::Result<std::size_t> readBytes(const lvr2::io::storage::DataKey& key,
                                                     std::span<std::byte> output) const override
    {
        const auto found = bytes_.find({key.group, key.name});
        if (found == bytes_.end())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::NotFound,
                                                  "byte dataset missing",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        if (output.size() < found->second.size())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::InvalidArgument,
                                                  "output byte span is too small",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        std::copy(found->second.begin(), found->second.end(), output.begin());
        return found->second.size();
    }

    lvr2::io::storage::Status writeBytes(const lvr2::io::storage::DataKey& key,
                                         std::span<const std::byte> bytes) override
    {
        bytes_[{key.group, key.name}] = std::vector<std::byte>(bytes.begin(), bytes.end());
        return {};
    }

    lvr2::io::storage::Result<lvr2::io::storage::MetaValue> readMeta(const lvr2::io::storage::MetaKey& key) const override
    {
        const auto found = metas_.find({key.group, key.name});
        if (found == metas_.end())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::NotFound,
                                                  "metadata missing",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        return found->second;
    }

    lvr2::io::storage::Status writeMeta(const lvr2::io::storage::MetaKey& key,
                                        const lvr2::io::storage::MetaValue& value) override
    {
        metas_[{key.group, key.name}] = value;
        return {};
    }

    lvr2::io::storage::Result<lvr2::PointBufferPtr> readPointBuffer(const lvr2::io::storage::DataKey& key) const override
    {
        const auto found = points_.find({key.group, key.name});
        if (found == points_.end())
        {
            return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::NotFound,
                                                  "point buffer missing",
                                                  uri_,
                                                  key.group,
                                                  key.name});
        }
        return found->second;
    }

    lvr2::io::storage::Status writePointBuffer(const lvr2::io::storage::DataKey& key,
                                               const lvr2::PointBufferPtr& buffer) override
    {
        points_[{key.group, key.name}] = buffer;
        return {};
    }

    lvr2::io::storage::Status writeFloatArray(const lvr2::io::storage::DataKey& key,
                                              const lvr2::io::storage::FloatArrayView& array) override
    {
        (void)array;
        return lvr2::io::storage::unexpected({lvr2::io::storage::ErrorCode::Unsupported,
                                              "float arrays are not implemented by the fake backend",
                                              uri_,
                                              key.group,
                                              key.name});
    }

private:
    bool hasGroup(const std::string& group) const
    {
        for (const auto& entry : metas_)
        {
            if (entry.first.first == group || entry.first.first.rfind(group + "/", 0) == 0)
            {
                return true;
            }
        }
        for (const auto& entry : points_)
        {
            if (entry.first.first == group || entry.first.first.rfind(group + "/", 0) == 0)
            {
                return true;
            }
        }
        for (const auto& entry : bytes_)
        {
            if (entry.first.first == group || entry.first.first.rfind(group + "/", 0) == 0)
            {
                return true;
            }
        }
        return false;
    }

    using Key = std::pair<std::string, std::string>;

    std::string uri_;
    std::map<Key, lvr2::io::storage::MetaValue> metas_;
    std::map<Key, lvr2::PointBufferPtr> points_;
    std::map<Key, std::vector<std::byte>> bytes_;
};

lvr2::io::storage::Result<std::unique_ptr<lvr2::io::storage::StorageBackend>> openMemory(
    const lvr2::io::storage::OpenRequest& request)
{
    return std::unique_ptr<lvr2::io::storage::StorageBackend>(new MemoryBackend(request.uri));
}

} // namespace

TEST(StorageRegistryProjectStore, FakeBackendUsesRegistryPath)
{
    lvr2::io::storage::StorageRegistry registry;
    ASSERT_TRUE(registry.add(lvr2::io::storage::StorageKind::named("fake"), openMemory));

    auto options = lvr2::io::scan::LoadOptions::directory_raw_ply();
    options.kind = lvr2::io::storage::StorageKind::named("fake");
    options.loadMode = lvr2::io::storage::LoadMode::Eager;

    auto opened = lvr2::io::scan::open_project("memory://project", options, registry);
    ASSERT_TRUE(opened) << opened.error().message;

    auto project = makeProject();
    ASSERT_TRUE(opened.value().save(*project));
    auto loaded = opened.value().load();
    ASSERT_TRUE(loaded) << loaded.error().message;
    expectProjectRoundTrip(loaded.value());
}

TEST(StorageRegistryProjectStore, RejectsUnsupportedCameraPayloads)
{
    lvr2::io::storage::StorageRegistry registry;
    ASSERT_TRUE(registry.add(lvr2::io::storage::StorageKind::named("fake"), openMemory));

    auto options = lvr2::io::scan::LoadOptions::directory_raw_ply();
    options.kind = lvr2::io::storage::StorageKind::named("fake");

    auto opened = lvr2::io::scan::open_project("memory://project", options, registry);
    ASSERT_TRUE(opened) << opened.error().message;

    auto project = makeProject();
    project->positions[0]->cameras.push_back(std::make_shared<lvr2::Camera>());

    auto saved = opened.value().save(*project);
    ASSERT_FALSE(saved);
    EXPECT_EQ(saved.error().code, lvr2::io::storage::ErrorCode::Unsupported);
}

TEST(StorageProjectStore, DirectoryRoundTrip)
{
    const auto root = uniqueTempPath("-directory");
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    auto opened = lvr2::io::scan::open_directory(root.string(),
                                                 lvr2::io::scan::Schema::raw_ply(),
                                                 lvr2::io::storage::LoadMode::Eager);
    ASSERT_TRUE(opened) << opened.error().message;

    auto project = makeProject();
    ASSERT_TRUE(opened.value().save(*project));
    auto loaded = opened.value().load();
    ASSERT_TRUE(loaded) << loaded.error().message;
    expectProjectRoundTrip(loaded.value());

    auto position = opened.value().load_position(1);
    ASSERT_TRUE(position) << position.error().message;
    ASSERT_TRUE(position.value());
    ASSERT_EQ(position.value()->lidars.size(), 1u);

    auto scan = opened.value().load_scan(1, 0, 0);
    ASSERT_TRUE(scan) << scan.error().message;
    ASSERT_TRUE(scan.value());
    ASSERT_TRUE(scan.value()->points || scan.value()->loadable());
    scan.value()->load();
    ASSERT_TRUE(scan.value()->points);
    EXPECT_EQ(scan.value()->points->numPoints(), 2u);

    std::filesystem::remove_all(root, ec);
}

TEST(StorageProjectStore, DirectoryRawPointDataFailsExplicitly)
{
    const auto root = uniqueTempPath("-directory-raw");
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    auto opened = lvr2::io::scan::open_directory(root.string(),
                                                 lvr2::io::scan::Schema::raw(),
                                                 lvr2::io::storage::LoadMode::Eager);
    ASSERT_TRUE(opened) << opened.error().message;

    auto project = makeProject();
    auto saved = opened.value().save(*project);
    ASSERT_FALSE(saved);
    EXPECT_EQ(saved.error().code, lvr2::io::storage::ErrorCode::Unsupported);

    std::filesystem::remove_all(root, ec);
}

TEST(StorageProjectStore, DirectoryRawPointDataLoadFailsExplicitly)
{
    const auto root = uniqueTempPath("-directory-raw-load");
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    auto opened = lvr2::io::scan::open_directory(root.string(),
                                                 lvr2::io::scan::Schema::raw(),
                                                 lvr2::io::storage::LoadMode::Eager);
    ASSERT_TRUE(opened) << opened.error().message;

    auto project = makeProject();
    project->positions[0]->lidars[0]->scans[0]->points.reset();
    ASSERT_TRUE(opened.value().save(*project));

    auto loaded = opened.value().load();
    ASSERT_FALSE(loaded);
    EXPECT_EQ(loaded.error().code, lvr2::io::storage::ErrorCode::Unsupported);

    std::filesystem::remove_all(root, ec);
}

TEST(StorageProjectStore, Hdf5RoundTrip)
{
    const auto file = uniqueTempPath(".h5");
    std::error_code ec;
    std::filesystem::remove(file, ec);

    auto opened = lvr2::io::scan::open_hdf5(file.string(),
                                            lvr2::io::scan::Schema::hdf5(),
                                            lvr2::io::storage::LoadMode::Eager);
    ASSERT_TRUE(opened) << opened.error().message;

    auto project = makeProject();
    ASSERT_TRUE(opened.value().save(*project));
    auto loaded = opened.value().load();
    ASSERT_TRUE(loaded) << loaded.error().message;
    expectProjectRoundTrip(loaded.value());

    auto lidar = opened.value().load_lidar(1, 0);
    ASSERT_TRUE(lidar) << lidar.error().message;
    ASSERT_TRUE(lidar.value());
    ASSERT_EQ(lidar.value()->scans.size(), 1u);

    std::filesystem::remove(file, ec);
}
