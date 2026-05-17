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


#ifndef BASEBUFFER
#define BASEBUFFER

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "MultiChannelMap.hpp"
#include "lvr2/io/DataStruct.hpp"

namespace lvr2 {

using intOptional = boost::optional<int>;
using floatOptional = boost::optional<float>;
using ucharOptional = boost::optional<unsigned char>;

using FloatProxy = ElementProxy<float>;
using UCharProxy = ElementProxy<unsigned char>;
using IndexProxy = ElementProxy<unsigned int>;

/**
 * @brief ChannelManager class
 *      Store and access AttributeChannels. It expands the MultiChannelMap with
 *      downwoards compitibility functions of the old ChannelManager.
 * 
 */
class BaseBuffer : public MultiChannelMap {
using base = MultiChannelMap;
public:
    using base::base;
    virtual ~BaseBuffer(){};

    //////////////////////////////////
    //// Width Channel functions ////
    //////////////////////////////////

    /**
     * @brief Gets a channels width.
     * @param[in] name Key of the channel.
     * @tparam T type of channel to search for.
     * @return 0 if not found, otherwise the channels width.
     */
    template<typename T>
    size_t channelWidth(std::string_view name) const;

    /**
     * @brief Gets an uchar channels width.
     * @param[in] name Key of the channel.
     * @return 0 if not found, otherwise the channels width.
     */
    inline size_t ucharChannelWidth(std::string_view name) const
    {
        return channelWidth<unsigned char>(name);
    }

    /**
     * @brief Gets a float channels width.
     * @param[in] name Key of the channel.
     * @return 0 if not found, otherwise the channels width.
     */
    inline size_t floatChannelWidth(std::string_view name) const
    {
        return channelWidth<float>(name);
    }

    /**
     * @brief Gets an index channels width.
     * @param[in] name Key of the channel.
     * @return 0 if not found, otherwise the channels width.
     */
    inline size_t indexChannelWidth(std::string_view name) const
    {
        return channelWidth<unsigned int>(name);
    }

    //////////////////////////////////
    //// Has Channel functions ////
    //////////////////////////////////

    /**
     * @brief Checks if a channel is available.
     * @param[in] name Key of the channel.
     * @tparam T Type of the channel.
     */
    template<typename T>
    bool hasChannel(std::string_view name) const;

    /**
     * @brief Checks if an uchar channel is available.
     * @param[in] name Key of the channel.
     */
    inline bool hasUCharChannel(std::string_view name) const
    {
        return hasChannel<unsigned char>(name);
    }

    /**
     * @brief Checks if a float channel is available.
     * @param[in] name Key of the channel.
     */
    inline bool hasFloatChannel(std::string_view name) const
    {
        return hasChannel<float>(name);
    }

    /**
     * @brief Checks if an index channel is available.
     * @param[in] name Key of the channel.
     */
    inline bool hasIndexChannel(std::string_view name) const
    {
        return hasChannel<unsigned int>(name);
    }

    //////////////////////////////////
    //// Add Channel functions ////
    //////////////////////////////////

    /**
     * @brief Adds a channel pointer to the map.
     * @param[in] data The channel pointer to add. 
     * @param[in] name The key of the channel.
     * @tparam T Type of the channel.
     */
    template<typename T>
    void addChannel(typename Channel<T>::Ptr data, std::string_view name);

    /**
     * @brief Adds a float channel pointer to the map.
     * @param[in] data The channel pointer to add. 
     * @param[in] name Key of the channel.
     */
    inline void addFloatChannel(FloatChannelPtr data, std::string_view name)
    {
        addChannel<float>(data, name);
    }

    /**
     * @brief Adds an uchar channel pointer to the map.
     * @param[in] data The channel pointer to add. 
     * @param[in] name Key of the channel.
     */
    inline void addUCharChannel(UCharChannelPtr data, std::string_view name)
    {
        addChannel<unsigned char>(data, name);
    }

    /**
     * @brief Adds an index channel pointer to the map.
     * cointer to add. 
     * cannel.
     */
    inline void addIndexChannel(IndexChannelPtr data, std::string_view name)
    {
        addChannel<unsigned int>(data, name);
    }

    /**
     * @brief Constructs a channel from an boost::shared_array and saves it to the map.
     * @param[in] array The shared array of the data. 
     * @param[in] name Key of the channel.
     * @tparam T Type of the channel.
     */
    template<typename T>
    void addChannel(boost::shared_array<T> array, std::string_view name, size_t n, size_t width);

    /**
     * @brief Copies a contiguous span into an owned channel and saves it to the map.
     * @param[in] values The contiguous source values. The span is not stored.
     * @param[in] name Key of the channel.
     * @param[in] n Number of elements.
     * @param[in] width Width of one element.
     * @tparam T Type of the channel.
     */
    template<typename T>
    void addChannel(std::span<const T> values, std::string_view name, size_t n, size_t width);

    /**
     * @brief Constructs an index channel from an boost::shared_array and saves it to the map.
     * @param[in] array The shared array of the data. 
     * @param[in] name Key of the channel.
     */
    inline void addIndexChannel( indexArray array, std::string_view name, size_t n, size_t width)
    {
        addChannel<unsigned int>(array, name, n, width);
    }

    /**
     * @brief Copies contiguous index values into an owned channel and saves it to the map.
     */
    inline void addIndexChannel(std::span<const unsigned int> values, std::string_view name, size_t n, size_t width)
    {
        addChannel<unsigned int>(values, name, n, width);
    }

    /**
     * @brief Constructs a float channel from an boost::shared_array and saves it to the map.
     * @param[in] array The shared array of the data. 
     * @param[in] name Key of the channel.
     */
    inline void addFloatChannel( floatArr array, std::string_view name, size_t n, size_t width)
    {
        addChannel<float>(array, name, n, width);
    }

    /**
     * @brief Copies contiguous float values into an owned channel and saves it to the map.
     */
    inline void addFloatChannel(std::span<const float> values, std::string_view name, size_t n, size_t width)
    {
        addChannel<float>(values, name, n, width);
    }

    /**
     * @brief Constructs an uchar channel from an boost::shared_array
     *          and saves it to the map.
     * 
     * @param[in] array The shared array of the data. 
     * @param[in] name Key of the channel.
     */
    inline void addUCharChannel(ucharArr array, std::string_view name, size_t n, size_t width)
    {
        addChannel<unsigned char>(array, name, n, width);
    }

    /**
     * @brief Copies contiguous uchar values into an owned channel and saves it to the map.
     */
    inline void addUCharChannel(std::span<const unsigned char> values, std::string_view name, size_t n, size_t width)
    {
        addChannel<unsigned char>(values, name, n, width);
    }

    /**
     * @brief Adds an empty channel to the map.
     * 
     * @param[in] name Key of the channel.
     * @param[in] n Number of elements.
     * @param[in] width Width of one element.
     * @tparam T Type of the channel.
     */
    template<typename T>
    void addEmptyChannel( std::string_view name, size_t n, size_t width);

    /**
     * @brief Adds an empty float channel to the map.
     * @param[in] name Key of the channel.
     * @param[in] n Number of elements.
     * @param[in] width Width of one element.
     */
    inline void addEmptyFloatChannel( std::string_view name, size_t n, size_t width)
    {
        addEmptyChannel<float>(name, n, width);
    }

    /**
     * @brief Adds an empty uchar channel to the map.
     * @param[in] name Key of the channel.
     * @param[in] n Number of elements.
     * @param[in] width Width of one element.
     */
    inline void addEmptyUCharChannel( std::string_view name, size_t n, size_t width)
    {
        addEmptyChannel<unsigned char>(name, n, width);
    }

    /**
     * @brief Adds an empty index channel to the map.
     * @param[in] name Key of the channel.
     * @param[in] n Number of elements.
     * @param[in] width Width of one element.
     */
    inline void addEmptyIndexChannel( std::string_view name, size_t n, size_t width)
    {
        addEmptyChannel<unsigned int>(name, n, width);
    }

    //////////////////////////////////
    //// Remove Channel functions ////
    //////////////////////////////////
    
    /**
     * @brief Removes a channel with a specific type.
     * @detail If the type is not required use: erase.
     * 
     * @tparam T Type of the channel.
     * @param[in] name Key of the channel.
     * @return true If the channel was removed.
     * @return false If no channel was removed.
     */
    template<typename T>
    bool removeChannel(std::string_view name);

    /**
     * @brief Removes an index channel.
     * @detail If the type is not required use: erase.
     * 
     * @param[in] name Key of the channel.
     * @return true If the channel was removed.
     * @return false If no channel was removed.
     */
    bool removeIndexChannel(std::string_view name)
    {
        return removeChannel<unsigned int>(name);
    }

    /**
     * @brief Removes a float channel.
     * @detail If the type is not required use: erase.
     * 
     * @param[in] name Key of the channel.
     * @return true If the channel was removed.
     * @return false If no channel was removed.
     */
    bool removeFloatChannel(std::string_view name)
    {
        return removeChannel<float>(name);
    }

    /**
     * @brief Removes an uchar channel.
     * @detail If the type is not required use: erase.
     * 
     * @param[in] name Key of the channel.
     * @return true If the channel was removed.
     * @return false If no channel was removed.
     */
    bool removeUCharChannel(std::string_view name)
    {
        return removeChannel<unsigned char>(name);
    }

    ///////////////////////////////
    //// Get Channel functions ////
    ///////////////////////////////
    //
   
    /**
     * @brief Returns all channels of type T.
     *
     * @tparam T The type of the channels.
     * @param channels The vector of channel pairs(name, Channel).
     *
     * @return The type index in the MultiChannelMap.
     */
    template <typename T>
    int getAllChannelsOfType(std::map<std::string, Channel<T> >& channels)
    {
        for(auto it = this->typedBegin<T>(); it != this->end(); ++it)
        {
            channels.insert(*it);
        }
        return index_of_type<T>::value;
    }

    /**
     * @brief Returns all channels of type T.
     *
     * @tparam T The type of the channels.
     * @param channels The vector of channel pairs(name, Channel).
     *
     * @return The type index in the MultiChannelMap.
     */
    template <typename T>
    int getAllChannelsOfType(std::vector<std::pair<std::string, Channel<T> >>& channels)
    {
        for(auto it = this->typedBegin<T>(); it != this->end(); ++it)
        {
            channels.push_back({it->first, it->second});
        }
        return index_of_type<T>::value;
    }


    /**
     * @brief Gets a channel and returns it as optional. 
     * 
     * @param[in] name Key of the channel.
     * @tparam T Type of the channel.
     * @return An OptionalChannel which is filled if the channel was found.
     */
    template<typename T>
    typename Channel<T>::Optional getChannel(std::string_view name);

        /**
     * @brief Gets a channel and returns it as optional. 
     * 
     * @param[in] name Key of the channel.
     * @tparam T Type of the channel.
     * @return An OptionalChannel which is filled if the channel was found.
     */
    template<typename T>
    const typename Channel<T>::Optional getChannel(std::string_view name) const;


    /**
     * @brief Gets a float channel and returns it as optional. 
     * 
     * @param[in] name Key of the channel.
     * @return An OptionalChannel which is filled if the channel was found.
     */
    inline Channel<float>::Optional getFloatChannel(std::string_view name)
    {
        return getChannel<float>(name);
    }

    /**
     * @brief Gets an uchar channel and returns it as optional. 
     * 
     * @param[in] name Key of the channel.
     * @return An OptionalChannel which is filled if the channel was found.
     */
    inline Channel<unsigned char>::Optional getUCharChannel(std::string_view name)
    {
        return getChannel<unsigned char>(name);
    }

    /**
     * @brief Gets an index channel and returns it as optional. 
     * 
     * @param[in] name Key of the channel.
     * @return An OptionalChannel which is filled if the channel was found.
     */
    inline Channel<unsigned int>::Optional getIndexChannel(std::string_view name)
    {
        return getChannel<unsigned int>(name);
    }

    /**
     * @brief Gets a float channel and returns it as optional.
     * 
     * @param[in] name Key of the channel.
     * @param[out] channelOptional The float channel optional.
     */
    inline void getChannel(std::string_view name, FloatChannelOptional& channelOptional )
    { 
        channelOptional = getFloatChannel(name);
    }

    /**
     * @brief Gets an index channel and returns it as optional.
     * 
     * @param[in] name Key of the channel.
     * @param[out] channelOptional The index channel optional.
     */
    inline void getChannel(std::string_view name, IndexChannelOptional& channelOptional)
    {
        channelOptional = getIndexChannel(name);
    }

    /**
     * @brief Gets an uchar channel and returns it as optional.
     * 
     * @param[in] name Key of the channel.
     * @param[out] channelOptional The uchar channel optional.
     */
    inline void getChannel(std::string_view name, UCharChannelOptional& channelOptional)
    {
        channelOptional = getUCharChannel(name);
    }

    ///////////////////////////////
    //// Get Handle functions ////
    ///////////////////////////////

    /**
     * @brief Get a Handle object (ElementProxy) of a specific typed channel.
     * 
     * @tparam T The type of the channel.
     * @param[in] idx The index of the element to access.
     * @param[in] name Key of the channel.
     * @return ElementProxy<T> The handle.
     */
    template<typename T>
    ElementProxy<T> getHandle(unsigned int idx, std::string_view name);
    
    /**
     * @brief Get a Handle object (ElementProxy) of a float channel.
     * 
     * @param[in] idx The index of the element to access.
     * @param[in] name Key of the channel.
     * @return FloatProxy The handle.
     */
    inline FloatProxy getFloatHandle(unsigned int idx, std::string_view name)
    {
        return getHandle<float>(idx, name);
    }

    /**
     * @brief Get a Handle object (ElementProxy) of an uchar channel.
     * 
     * @param[in] idx The index of the element to access.
     * @param[in] name Key of the channel.
     * @return UCharProxy The handle.
     */
    inline UCharProxy getUCharHandle(unsigned int idx, std::string_view name)
    {
        return getHandle<unsigned char>(idx, name);
    }
    
    /**
     * @brief Get a Handle object (ElementProxy) of an index channel.
     * 
     * @param[in] idx The index of the element to access.
     * @param[in] name Key of the channel.
     * @return IndexProxy The handle.
     */
    inline IndexProxy getIndexHandle(unsigned int idx, std::string_view name)
    {
        return getHandle<unsigned int>(idx, name);
    }

    ///////////////////////////////
    //// Get Array functions ////
    ///////////////////////////////

    /**
     * @brief Gets a channel as array.
     * 
     * @tparam T Type of the channel.
     * @param[out] n Number of elements stored in the channel.
     * @param[out] w Width of an element.
     * @param[in] name Key of the channel.
     * @return boost::shared_array<T> The data pointer. Empty if the channel was not found.
     */
    template<typename T>
    boost::shared_array<T> getArray(std::string_view name, size_t& n, size_t& w);

    /**
     * @brief Gets a float channel as array.
     * 
     * @param[out] n Number of elements stored in the channel.
     * @param[out] w Width of an element.
     * @param[in] name Key of the channel.
     * @return floatArr The data pointer. Empty if the channel was not found.
     */
    inline floatArr getFloatArray(std::string_view name, size_t& n, size_t& w)
    {
        return getArray<float>(name, n, w);
    }

    /**
     * @brief Gets an uchar channel as array.
     * 
     * @param[out] n Number of elements stored in the channel.
     * @param[out] w Width of an element.
     * @param[in] name Key of the channel.
     * @return ucharArr The data pointer. Empty if the channel was not found.
     */
    inline ucharArr getUCharArray(std::string_view name, size_t& n, size_t& w)
    {
        return getArray<unsigned char>(name, n, w);
    }

    /**
     * @brief Gets an index channel as array.
     * 
     * @param[out] n Number of elements stored in the channel.
     * @param[out] w Width of an element.
     * @param[in] name Key of the channel.
     * @return indexArray The data pointer. Empty if the channel was not found.
     */
    inline indexArray getIndexArray(std::string_view name, size_t& n, size_t& w)
    {
        return getArray<unsigned int>(name, n, w);
    }

    ///////////////////////////////
    //// Add Atomic functions ////
    ///////////////////////////////

    /**
     * @brief Adds an atomic value. Exists only for compatibility reasons.
     *          Dont use atomics, they are implemented as Channels.
     *          -> memory overhead.
     * 
     * @tparam T Type of the atomic value.
     * @param[in] data The atomic data to add to the channel manager.
     * @param[in] name The key of the atomic value (don't use keys that are already used for channels).
     */
    template<typename T>
    void addAtomic(T data, std::string_view name);

    /**
     * @brief Adds an atomic float value. Exists only for compatibility reasons.
     *          Dont use atomics, they are implemented as Channels.
     *          -> memory overhead.
     * @param[in] data The atomic data to add to the channel manager.
     * @param[in] name The key of the atomic value (don't use keys that are already used for channels).
     */
    inline void addFloatAtomic(float data, std::string_view name)
    {
        addAtomic(data, name);
    }

    /**
     * @brief Adds an atomic uchar value. Exists only for compatibility reasons.
     *          Dont use atomics, they are implemented as Channels.
     *          -> memory overhead.
     *          Kept because of api stability.
     * @param[in] data The atomic data to add to the channel manager.
     * @param[in] name The key of the atomic value (don't use keys that are already used for channels).
     */
    inline void addUCharAtomic(unsigned char data, std::string_view name)
    {
        addAtomic(data, name);
    }

    /**
     * @brief Adds an atomic int value. Exists only for compatibility reasons.
     *          Dont use atomics, they are implemented as Channels.
     *          -> memory overhead.
     *          Kept because of api stability.
     * @param[in] data The atomic data to add to the channel manager.
     * @param[in] name The key of the atomic value (don't use keys that are already used for channels).
     */
    inline void addIntAtomic(int data, std::string_view name)
    {
        addAtomic(data, name);
    }
    ///////////////////////////////
    //// Get Atomic functions ////
    ///////////////////////////////

    /**
     * @brief Gets an atomic value.
     * 
     * @tparam T The atomic values type.
     * @param[in] name Key of the atomic value. 
     * 
     * @return The atomic value as optional. The optional is set if the atomic value was found.
     * 
     */
    template<typename T>
    boost::optional<T> getAtomic(std::string_view name);

    /**
     * @brief Gets an atomic float value.
     * 
     * @param[in] name Key of the atomic value. 
     * 
     * @return The atomic value as optional. The optional is set if the atomic value was found.
     * 
     */
    inline floatOptional getFloatAtomic(std::string_view name)
    {
        return getAtomic<float>(name);
    }

    /**
     * @brief Gets an atomic uchar value.
     * 
     * @param[in] name Key of the atomic value. 
     * 
     * @return The atomic value as optional. The optional is set if the atomic value was found.
     * 
     */
    inline ucharOptional getUCharAtomic(std::string_view name)
    {
        return getAtomic<unsigned char>(name);
    }

    /**
     * @brief Gets an atomic int value.
     * 
     * @param[in] name Key of the atomic value. 
     * 
     * @return The atomic value as optional. The optional is set if the atomic value was found.
     * 
     */
    inline intOptional getIntAtomic(std::string_view name)
    {
        return getAtomic<int>(name);
    }

    template<typename V>
    BaseBuffer manipulate(V visitor)
    {
        BaseBuffer cm;
        for(auto vchannel: *this)
        {
            cm.insert({vchannel.first, boost::apply_visitor(visitor, vchannel.second)});
        }
        return cm;
    }

    BaseBuffer clone() const {
        BaseBuffer ret;

        for(auto elem : *this)
        {
            ret.insert({elem.first, elem.second.clone()});
        }

        return ret;
    }
};

} // namespace lvr2

#include "BaseBuffer.tcc"

#endif // BASEBUFFER
