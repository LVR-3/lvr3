#ifndef LVR2_IO_STORAGE_CHUNK_STORE_HPP
#define LVR2_IO_STORAGE_CHUNK_STORE_HPP

#include "lvr2/geometry/BaseVector.hpp"
#include "lvr2/geometry/BoundingBox.hpp"
#include "lvr2/types/MeshBuffer.hpp"
#include "lvr2/types/PointBuffer.hpp"

#include <memory>
#include <string>
#include <type_traits>

namespace lvr2::io::storage
{

class ChunkStore final
{
public:
    ChunkStore();
    explicit ChunkStore(const std::string& filename);
    ~ChunkStore();

    ChunkStore(ChunkStore&&) noexcept;
    ChunkStore& operator=(ChunkStore&&) noexcept;
    ChunkStore(const ChunkStore&) = delete;
    ChunkStore& operator=(const ChunkStore&) = delete;

    void open(const std::string& filename);

    void saveAmount(lvr2::BaseVector<std::size_t> amount);
    void saveChunkSize(float chunkSize);
    void saveBoundingBox(lvr2::BoundingBox<lvr2::BaseVector<float>> boundingBox);
    void save(lvr2::BaseVector<std::size_t> amount,
              float chunkSize,
              lvr2::BoundingBox<lvr2::BaseVector<float>> boundingBox);

    lvr2::BaseVector<std::size_t> loadAmount();
    float loadChunkSize();
    lvr2::BoundingBox<lvr2::BaseVector<float>> loadBoundingBox();

    template<typename T>
    void saveChunk(T data, const std::string& layer, int x, int y, int z)
    {
        if constexpr (std::is_same<T, lvr2::MeshBufferPtr>::value)
        {
            saveMeshChunk(data, layer, x, y, z);
        }
        else if constexpr (std::is_same<T, lvr2::PointBufferPtr>::value)
        {
            savePointChunk(data, layer, x, y, z);
        }
        else
        {
            static_assert(dependent_false<T>::value, "unsupported chunk payload type");
        }
    }

    template<typename T>
    T loadChunk(const std::string& layer, int x, int y, int z)
    {
        if constexpr (std::is_same<T, lvr2::MeshBufferPtr>::value)
        {
            return loadMeshChunk(layer, x, y, z);
        }
        else if constexpr (std::is_same<T, lvr2::PointBufferPtr>::value)
        {
            return loadPointChunk(layer, x, y, z);
        }
        else
        {
            static_assert(dependent_false<T>::value, "unsupported chunk payload type");
        }
    }

private:
    template<typename>
    struct dependent_false : std::false_type {};

    struct State;

    void saveMeshChunk(const lvr2::MeshBufferPtr& data, const std::string& layer, int x, int y, int z);
    void savePointChunk(const lvr2::PointBufferPtr& data, const std::string& layer, int x, int y, int z);
    lvr2::MeshBufferPtr loadMeshChunk(const std::string& layer, int x, int y, int z);
    lvr2::PointBufferPtr loadPointChunk(const std::string& layer, int x, int y, int z);

    std::unique_ptr<State> state_;
};

} // namespace lvr2::io::storage

#endif // LVR2_IO_STORAGE_CHUNK_STORE_HPP
