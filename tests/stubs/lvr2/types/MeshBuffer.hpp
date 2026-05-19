#ifndef LVR2_TESTS_STUBS_TYPES_MESHBUFFER_HPP
#define LVR2_TESTS_STUBS_TYPES_MESHBUFFER_HPP

#include <cstddef>

namespace lvr2
{

class MeshBuffer
{
public:
    MeshBuffer() = default;
    MeshBuffer(std::size_t vertices, std::size_t faces)
        : m_vertices(vertices), m_faces(faces)
    {
    }

    std::size_t numVertices() const
    {
        return m_vertices;
    }

    std::size_t numFaces() const
    {
        return m_faces;
    }

private:
    std::size_t m_vertices = 0;
    std::size_t m_faces = 0;
};

} // namespace lvr2

#endif // LVR2_TESTS_STUBS_TYPES_MESHBUFFER_HPP
