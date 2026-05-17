#ifndef LVR2_IO_MESH_STORES_HPP
#define LVR2_IO_MESH_STORES_HPP

#include "lvr2/io/AttributeMeshIOBase.hpp"
#include "lvr2/io/kernels/FileKernel.hpp"
#include "lvr2/io/schema/MeshSchema.hpp"
#include "lvr2/types/MeshBuffer.hpp"
#include "lvr2/util/Hdf5Util.hpp"

#include <optional>

#include <highfive/H5File.hpp>

#include <memory>
#include <string>
#include <vector>

namespace lvr2::io::mesh
{

class KernelMeshStore final
{
public:
    KernelMeshStore(lvr2::FileKernelPtr kernel, lvr2::MeshSchemaPtr schema);

    void save_mesh(const std::string& meshName, const lvr2::MeshBufferPtr& mesh) const;
    lvr2::MeshBufferPtr load_mesh(const std::string& meshName) const;
    std::vector<std::string> available_meshes() const;

private:
    lvr2::FileKernelPtr kernel_;
    lvr2::MeshSchemaPtr schema_;
};

class Hdf5MeshStore final : public lvr2::AttributeMeshIOBase
{
public:
    Hdf5MeshStore();
    explicit Hdf5MeshStore(const std::string& filename);

    void open(const std::string& filename);
    bool is_open() const;

    void set_mesh_name(std::string meshName);
    const std::string& mesh_name() const noexcept { return meshName_; }

    void save_mesh(const std::string& name, const lvr2::MeshBufferPtr& buffer);
    void save_mesh(HighFive::Group& group, const lvr2::MeshBufferPtr& buffer);
    lvr2::MeshBufferPtr load_mesh(const std::string& name);
    lvr2::MeshBufferPtr load_mesh(HighFive::Group& group);

    FloatChannelOptional getVertices() override;
    IndexChannelOptional getIndices() override;
    bool addVertices(const FloatChannel& channel) override;
    bool addIndices(const IndexChannel& channel) override;

    bool getChannel(const std::string group,
                    const std::string name,
                    FloatChannelOptional& channel) override;
    bool getChannel(const std::string group,
                    const std::string name,
                    IndexChannelOptional& channel) override;
    bool getChannel(const std::string group,
                    const std::string name,
                    UCharChannelOptional& channel) override;

    bool addChannel(const std::string group,
                    const std::string name,
                    const FloatChannel& channel) override;
    bool addChannel(const std::string group,
                    const std::string name,
                    const IndexChannel& channel) override;
    bool addChannel(const std::string group,
                    const std::string name,
                    const UCharChannel& channel) override;

private:
    bool is_mesh(HighFive::Group& group) const;
    void ensure_mesh_group();

    bool compress_ = true;
    std::size_t chunkSize_ = 10000000;
    std::string filename_;
    std::string meshName_;
    std::shared_ptr<HighFive::File> file_;
};

} // namespace lvr2::io::mesh

#endif // LVR2_IO_MESH_STORES_HPP
