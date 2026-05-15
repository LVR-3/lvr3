/**
 * Copyright (c) 2018, University Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University Osnabrück nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL University Osnabrück BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * PointsetSurface.tcc
 *
 *  @date 25.01.2012
 *  @author Thomas Wiemann
 */

namespace lvr2
{

template<typename BaseVecT>
PointsetSurface<BaseVecT>::PointsetSurface(PointBufferPtr pointBuffer)
: m_pointBuffer(pointBuffer)
, m_points(pointBuffer->at("points").extract<float>()) // points field is required
{
    // if no normals existing: create them
    // auto n_it = pointBuffer->find("normals");
    // if(n_it == pointBuffer->end())
    // {
    //     Channel<float> normals(m_points.numElements(), 3);
    //     pointBuffer->add("normals", normals);
    //     n_it = pointBuffer->find("normals");
    // }
    // m_normals = &n_it->second.extract<float>();

    // Calculate bounding box
    for (size_t i = 0; i < m_points.numElements(); i++)
    {
        this->m_boundingBox.expand(BaseVecT(m_points[i][0], m_points[i][1], m_points[i][2]));
    }
}

template<typename BaseVecT>
Normal<float> PointsetSurface<BaseVecT>::getInterpolatedNormal(const BaseVecT& position) const
{
    FloatChannelOptional normals = m_pointBuffer->getFloatChannel("normals");
    if(!normals)
    {
        return Normal<float>(0.0f, 0.0f, 1.0f);
    }

    std::vector<size_t> indices;
    m_searchTree->kSearch(position, m_ki, indices);

    auto isValidNormal = [](const BaseVecT& normal) {
        return std::isfinite(normal.x)
            && std::isfinite(normal.y)
            && std::isfinite(normal.z)
            && normal.length2() > std::numeric_limits<typename BaseVecT::CoordType>::epsilon();
    };

    auto normalizedNormal = [](BaseVecT normal) {
        normal /= normal.length();
        return normal;
    };

    BaseVecT reference;
    std::size_t referenceIndex = m_points.numElements();
    bool hasReference = false;
    for(const auto& index : indices)
    {
        if(index >= m_points.numElements())
        {
            continue;
        }

        const BaseVecT candidate = (*normals)[index];
        if(isValidNormal(candidate))
        {
            reference = normalizedNormal(candidate);
            referenceIndex = index;
            hasReference = true;
            break;
        }
    }

    if(!hasReference)
    {
        return Normal<float>(0.0f, 0.0f, 1.0f);
    }

    BaseVecT result = reference;
    for(const auto& index : indices)
    {
        if(index >= m_points.numElements() || index == referenceIndex)
        {
            continue;
        }

        BaseVecT normal = (*normals)[index];
        if(!isValidNormal(normal))
        {
            continue;
        }

        normal = normalizedNormal(normal);
        if(normal.dot(reference) < 0)
        {
            normal *= static_cast<typename BaseVecT::CoordType>(-1);
        }
        result += normal;
    }

    if(!isValidNormal(result))
    {
        result = reference;
    }

    return Normal<float>(
        static_cast<float>(result.x),
        static_cast<float>(result.y),
        static_cast<float>(result.z)
    );
}

template<typename BaseVecT>
std::shared_ptr<SearchTree<BaseVecT>> PointsetSurface<BaseVecT>::searchTree() const
{
    return m_searchTree;
}

template<typename BaseVecT>
const BoundingBox<BaseVecT>& PointsetSurface<BaseVecT>::getBoundingBox() const
{
    return m_boundingBox;
}

template<typename BaseVecT>
PointBufferPtr PointsetSurface<BaseVecT>::pointBuffer() const
{
    return m_pointBuffer;
}

} // namespace lvr2
