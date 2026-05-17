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

namespace lvr2 {

template<typename T>
size_t BaseBuffer::channelWidth(std::string_view name) const
{
    auto it = this->find(name);
    if(it != this->end() && it->second.is_type<T>())
    {
        return it->second.width();
    }
    return 0;
}

template<typename T>
bool BaseBuffer::hasChannel(std::string_view name) const
{
    auto it = this->find(name);
    if(it != this->end() && it->second.is_type<T>())
    {
        return true;
    }
    return false;
}

template<typename T>
void BaseBuffer::addChannel(typename Channel<T>::Ptr data, std::string_view name)
{
    this->emplace(std::string(name), *data);
}

template<typename T>
void BaseBuffer::addChannel(
    std::shared_ptr<T[]> array,
    std::string_view name,
    size_t n,
    size_t width)
{
    this->emplace(std::string(name), Channel<T>(n, width, array));
}

template<typename T>
void BaseBuffer::addChannel(
    std::span<const T> values,
    std::string_view name,
    size_t n,
    size_t width)
{
    this->emplace(std::string(name), Channel<T>(n, width, values));
}

template<typename T>
void BaseBuffer::addEmptyChannel(
    std::string_view name,
    size_t n,
    size_t width)
{
    Channel<T> channel(n, width);
    // init zeros
    std::fill(channel.dataPtr().get(), channel.dataPtr().get() + n * width, 0);
    this->emplace(std::string(name), channel);
}

template<typename T>
bool BaseBuffer::removeChannel(std::string_view name)
{
    auto it = this->find(name);
    if(it != this->end() && it->second.is_type<T>())
    {
        erase(it);
        return true;
    }
    return false;
}

template<typename T>
typename Channel<T>::Optional BaseBuffer::getChannel(std::string_view name)
{
    return getOptional<T>(name);
}

template<typename T>
const typename Channel<T>::Optional BaseBuffer::getChannel(std::string_view name) const
{
    return getOptional<T>(name);
}

template<typename T>
ElementProxy<T> BaseBuffer::getHandle(unsigned int idx, std::string_view name)
{
    // std::cout << "WARNING: runtime critical access [BaseBuffer::getHandle]" << std::endl;
    auto it = this->find(name);
    if(it != this->end() && it->second.is_type<T>())
    {
        return std::get<Channel<T> >(it->second)[idx];
    }
    return ElementProxy<T>();
}

template<typename T>
std::shared_ptr<T[]> BaseBuffer::getArray(
    std::string_view name, size_t& n, size_t& w)
{
    auto it = this->find(name);
    if(it != this->end() && it->second.is_type<T>())
    {
        Channel<T> channel = std::get<Channel<T> >(it->second);
        n = channel.numElements();
        w = channel.width();
        return channel.dataPtr();
    } 
    n = 0;
    w = 0;
    return std::shared_ptr<T[]>();
}

template<typename T>
void BaseBuffer::addAtomic(T data, std::string_view name)
{
    Channel<T> channel(1, 1);
    channel[0][0] = data;
    this->emplace(std::string(name), channel);
}

template<typename T>
std::optional<T> BaseBuffer::getAtomic(std::string_view name)
{
    std::optional<T> ret;
    typename Channel<T>::Optional channel = getChannel<T>(name);
    if(channel)
    {
        ret = (*channel)[0][0];
    }
    return ret;
}



} // namespace lvr2