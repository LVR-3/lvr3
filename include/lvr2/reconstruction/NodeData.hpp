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

/*
 * NodeData.hpp
 *
 *  Created on: Dec 11, 2015
 *      Author: Isaak Mitschke
 */

#ifndef LAS_VEGAS_NODEDATA_H
#define LAS_VEGAS_NODEDATA_H
#include <chrono>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace lvr2
{

class NodeDataTimer
{
public:
    void resume()
    {
        if(!m_running)
        {
            m_started = Clock::now();
            m_running = true;
        }
    }

    void stop()
    {
        if(m_running)
        {
            m_elapsed += Clock::now() - m_started;
            m_running = false;
        }
    }

    std::string format() const
    {
        auto elapsed = m_elapsed;
        if(m_running)
        {
            elapsed += Clock::now() - m_started;
        }

        std::ostringstream out;
        out << std::chrono::duration<double>(elapsed).count() << "s";
        return out.str();
    }

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point m_started{};
    Clock::duration m_elapsed{};
    bool m_running = false;
};
/**
 * Class that contains the actual points of a node
 */
template <typename BaseVecT>
class NodeData
{
    static NodeDataTimer itimer;
    static NodeDataTimer otimer;
    static bool timer_init;

    // Iterator
    class Iterator;

  public:
    /**
     * Constructor
     * @param bufferSize amount of points to store in a buffer. when buffersize is reached all
     * points are stored to hdd
     */
    NodeData(size_t bufferSize);
    /**
     * copy constructor
     * @param origin
     * @return
     */
    NodeData(NodeData& origin);

    /**
     * assignment operator
     * @param origin
     * @return
     */
    NodeData& operator=(NodeData& origin);

    /**
     * sets data path
     * @param path
     */
    void open(std::string path);

    /**
     * returns path where data is stored on hdd
     * @return
     */
    const std::string& getDataPath() const { return m_dataPath; }

    /**
     * returns path to folder on hdd where data is stored
     * @return
     */
    const std::string getFolder() const { return std::to_string(c_tstamp).insert(0, "node-"); }

    /**
     * Iterator referring to first element
     * @return
     */
    Iterator begin();

    /**
     * Returns an iterator referring to the past-the-end element
     * @return
     */
    Iterator end();

    /**
     * removes the data file from hdd
     */
    void remove();

    /**
     * removes element at index i (not implemented)
     * @param i
     */
    void remove(unsigned int i);

    /**
     * add a vertex to the data
     * @param input
     */
    void add(BaseVecT input);

    void addNormal(BaseVecT input);

    /**
     * adds a vertex to buffer
     * @param input
     */
    void addBuffered(BaseVecT input);

    void addBufferedNormal(BaseVecT input);

    /**
     * writes buffer to file
     */
    void writeBuffer();

    /**
     * gets amount of points stored in write buffer
     * @return
     */
    size_t getMaxWriteBufferSize();
    /**
     * gets element at index
     * @return
     */
    BaseVecT get(int);

    BaseVecT getNormal(int);
    /**
     * gets next element
     * @return
     */
    BaseVecT next();

    /**
     * returns amount of elements stored in data
     * @return
     */
    size_t size();

    /**
     * returns amount of poiunts stored in buffer
     * @return
     */
    size_t getBufferSize();

    static void printTimer()
    {
        std::cout << "IO-Timer of Octree:" << std::endl
                  << "READ: " << itimer.format() << std::endl
                  << "WRITE: " << otimer.format() << std::endl;
    }

  private:
    // copys the data from origin to this
    void copy(NodeData& origin);
    // fills the buffer, copys from start_id to buffersize elements into buffer
    void fillBuffer(size_t start_id);
    void fillBufferNormal(size_t start_id);
    // path to data
    std::string m_dataPath;

    std::string m_dataPathNormal;
    // if m_gotSize is set, the m_size will be used, else the size will be calculated
    bool m_gotSize;
    // amount of points stored in data
    size_t m_size;
    // current id of the dataobject, used to generate file name
    int m_id;
    // input buffer
    std::vector<float> m_readBuffer;
    // output buffer
    std::vector<float> m_writeBuffer;

    std::vector<float> m_readBufferNormal;
    // output buffer
    std::vector<float> m_writeBufferNormal;

    // maximum buffer size
    size_t m_bufferSize;
    // current inputbuffer position
    size_t m_readBufferIndex;

    size_t m_readBufferIndexNormal;
    // last id
    static int c_last_id;
    // timestamp, of first creation
    static time_t c_tstamp;
};

/**
 * Iterator for NodeData
 */
template <typename BaseVecT>
class NodeData<BaseVecT>::Iterator
{
  public:
    Iterator(NodeData& nodeData, size_t pos) : m_NodeData(nodeData), m_pos(pos) {}
    Iterator(NodeData& nodeData) : m_NodeData(nodeData), m_pos(0) {}
    Iterator(const Iterator& copy) : m_pos(copy.m_pos), m_NodeData(copy.m_NodeData) {}

    Iterator operator++(int)
    {
        Iterator tmp(*this);
        operator++();
        return tmp;
    }

    bool operator==(const Iterator& rhs) { return m_pos == rhs.m_pos; }
    bool operator!=(const Iterator& rhs) { return m_pos != rhs.m_pos; }
    // Todo: more if needed
    void operator+(int i) { m_pos += i; }
    void operator-(int i) { m_pos -= i; }
    void operator++() { ++m_pos; }
    void operator--() { --m_pos; }

    BaseVecT operator*() { return m_NodeData.get(m_pos); }
    BaseVecT operator->() { return m_NodeData.get(m_pos); }

  private:
    NodeData<BaseVecT>& m_NodeData;
    size_t m_pos;
};

} // namespace lvr2

#include "NodeData.tcc"

#endif // LAS_VEGAS_NODEDATA_H
