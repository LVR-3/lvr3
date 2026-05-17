#include "lvr2/io/storage.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace lvr2::io::storage
{

StorageKind StorageKind::auto_detect()
{
    return StorageKind{};
}

StorageKind StorageKind::directory()
{
    return StorageKind{"directory"};
}

StorageKind StorageKind::hdf5()
{
    return StorageKind{"hdf5"};
}

StorageKind StorageKind::named(std::string name)
{
    return StorageKind{std::move(name)};
}

StorageContext::StorageContext(std::unique_ptr<StorageBackend> backend,
                               LoadMode loadMode)
    : backend(std::move(backend))
    , loadMode(loadMode)
{
}

Status StorageRegistry::addFactory(StorageKind kind, StorageFactory factory)
{
    if (kind.is_auto())
    {
        return unexpected({ErrorCode::InvalidArgument,
                           "storage kind must be explicit when registering a factory"});
    }
    if (!factory)
    {
        return unexpected({ErrorCode::InvalidArgument,
                           "storage factory must be callable"});
    }

    const auto inserted = factories_.emplace(std::move(kind), factory);
    if (!inserted.second)
    {
        return unexpected({ErrorCode::DuplicateKind,
                           "storage factory already registered"});
    }

    return {};
}

Result<std::unique_ptr<StorageBackend>> StorageRegistry::open(const OpenRequest& request) const
{
    if (request.uri.empty())
    {
        return unexpected({ErrorCode::EmptyPath,
                           "storage uri must not be empty",
                           request.uri});
    }

    if (!request.kind.is_auto())
    {
        const auto found = factories_.find(request.kind);
        if (found == factories_.end())
        {
            return unexpected({ErrorCode::UnknownKind,
                               "no storage factory registered for requested kind",
                               request.uri});
        }
        return found->second(request);
    }

    std::string lowerUri = request.uri;
    std::transform(lowerUri.begin(), lowerUri.end(), lowerUri.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const auto hasSuffix = [&lowerUri](const std::string& suffix) {
        return lowerUri.size() >= suffix.size() &&
               lowerUri.compare(lowerUri.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    OpenRequest explicitRequest = request;
    explicitRequest.kind = (hasSuffix(".h5") || hasSuffix(".hdf5"))
        ? StorageKind::hdf5()
        : StorageKind::directory();

    const auto found = factories_.find(explicitRequest.kind);
    if (found != factories_.end())
    {
        return found->second(explicitRequest);
    }

    for (const auto& entry : factories_)
    {
        explicitRequest.kind = entry.first;
        auto opened = entry.second(explicitRequest);
        if (opened)
        {
            return std::move(opened);
        }
    }

    return unexpected({ErrorCode::UnknownKind,
                       "no storage factory accepted the uri",
                       request.uri});
}

StorageRegistry make_default_registry()
{
    StorageRegistry registry;
    auto registeredDefault = register_default_backends(registry);
    (void)registeredDefault;
    return registry;
}

} // namespace lvr2::io::storage
