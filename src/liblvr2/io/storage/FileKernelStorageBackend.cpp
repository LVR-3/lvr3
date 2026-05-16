#include "lvr2/io/storage.hpp"

#include "lvr2/io/kernels/DirectoryKernel.hpp"
#include "lvr2/io/kernels/HDF5Kernel.hpp"

#include <yaml-cpp/yaml.h>

#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

namespace lvr2::io::storage
{
namespace
{

Error makeBackendError(ErrorCode code,
                       std::string message,
                       const BackendInfo& info,
                       std::string group = {},
                       std::string name = {})
{
    return {code, std::move(message), info.uri, std::move(group), std::move(name)};
}

class KernelStorageBackend final : public StorageBackend
{
public:
    KernelStorageBackend(BackendInfo info, lvr2::FileKernelPtr kernel)
        : info_(std::move(info))
        , kernel_(std::move(kernel))
    {
    }

    BackendInfo info() const override
    {
        return info_;
    }

    Result<bool> exists(const GroupKey& key) const override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group));
        }

        try
        {
            return kernel_->exists(key.group);
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::ReadFailed,
                                               e.what(),
                                               info_,
                                               key.group));
        }
    }

    Result<bool> exists(const DataKey& key) const override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group,
                                               key.name));
        }

        try
        {
            return kernel_->exists(key.group, key.name);
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::ReadFailed,
                                               e.what(),
                                               info_,
                                               key.group,
                                               key.name));
        }
    }

    Result<std::vector<std::string>> list(const GroupKey& key) const override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group));
        }

        try
        {
            return kernel_->listDatasets(key.group);
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::ReadFailed,
                                               e.what(),
                                               info_,
                                               key.group));
        }
    }

    Result<MetaValue> readMeta(const MetaKey& key) const override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group,
                                               key.name));
        }

        YAML::Node node;
        try
        {
            if (!kernel_->loadMetaYAML(key.group, key.name, node))
            {
                return unexpected(makeBackendError(ErrorCode::NotFound,
                                                   "metadata was not found",
                                                   info_,
                                                   key.group,
                                                   key.name));
            }
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::ReadFailed,
                                               e.what(),
                                               info_,
                                               key.group,
                                               key.name));
        }

        return MetaValue{YAML::Dump(node)};
    }

    Status writeMeta(const MetaKey& key, const MetaValue& value) override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group,
                                               key.name));
        }

        try
        {
            kernel_->saveMetaYAML(key.group, key.name, YAML::Load(value.text));
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::WriteFailed,
                                               e.what(),
                                               info_,
                                               key.group,
                                               key.name));
        }

        return {};
    }

    Result<lvr2::PointBufferPtr> readPointBuffer(const DataKey& key) const override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group,
                                               key.name));
        }

        try
        {
            if (!kernel_->exists(key.group, key.name))
            {
                return unexpected(makeBackendError(ErrorCode::NotFound,
                                                   "point buffer was not found",
                                                   info_,
                                                   key.group,
                                                   key.name));
            }

            auto points = kernel_->loadPointBuffer(key.group, key.name);
            if (!points)
            {
                return unexpected(makeBackendError(ErrorCode::NotFound,
                                                   "point buffer was not found",
                                                   info_,
                                                   key.group,
                                                   key.name));
            }
            return points;
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::ReadFailed,
                                               e.what(),
                                               info_,
                                               key.group,
                                               key.name));
        }
    }

    Status writePointBuffer(const DataKey& key, const lvr2::PointBufferPtr& buffer) override
    {
        if (!kernel_)
        {
            return unexpected(makeBackendError(ErrorCode::OpenFailed,
                                               "storage backend is not open",
                                               info_,
                                               key.group,
                                               key.name));
        }
        if (!buffer)
        {
            return unexpected(makeBackendError(ErrorCode::InvalidArgument,
                                               "point buffer must not be null",
                                               info_,
                                               key.group,
                                               key.name));
        }

        try
        {
            kernel_->savePointBuffer(key.group, key.name, buffer);
        }
        catch (const std::exception& e)
        {
            return unexpected(makeBackendError(ErrorCode::WriteFailed,
                                               e.what(),
                                               info_,
                                               key.group,
                                               key.name));
        }

        return {};
    }

private:
    BackendInfo info_;
    lvr2::FileKernelPtr kernel_;
};

Result<std::unique_ptr<StorageBackend>> openDirectoryKernel(const OpenRequest& request)
{
    try
    {
        BackendInfo info{StorageKind::directory(), request.uri, "directory"};
        lvr2::FileKernelPtr kernel = std::make_shared<lvr2::DirectoryKernel>(request.uri);
        return std::unique_ptr<StorageBackend>(new KernelStorageBackend(std::move(info), std::move(kernel)));
    }
    catch (const std::exception& e)
    {
        return unexpected({ErrorCode::OpenFailed, e.what(), request.uri});
    }
}

Result<std::unique_ptr<StorageBackend>> openHdf5Kernel(const OpenRequest& request)
{
    try
    {
        BackendInfo info{StorageKind::hdf5(), request.uri, "hdf5"};
        lvr2::FileKernelPtr kernel = std::make_shared<lvr2::HDF5Kernel>(request.uri);
        return std::unique_ptr<StorageBackend>(new KernelStorageBackend(std::move(info), std::move(kernel)));
    }
    catch (const std::exception& e)
    {
        return unexpected({ErrorCode::OpenFailed, e.what(), request.uri});
    }
}

} // namespace

Status register_default_backends(StorageRegistry& registry)
{
    auto directoryStatus = registry.add(StorageKind::directory(), openDirectoryKernel);
    if (!directoryStatus)
    {
        return directoryStatus;
    }

    auto hdf5Status = registry.add(StorageKind::hdf5(), openHdf5Kernel);
    if (!hdf5Status)
    {
        return hdf5Status;
    }

    return {};
}

} // namespace lvr2::io::storage
