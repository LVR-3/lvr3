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

#pragma once

#include "lvr2/types/MatrixTypes.hpp"

#include <array>
#include <filesystem>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace lvr2
{

namespace detail
{
class RieglXmlNode;
}

struct RieglImageFile
{
    fs::path image_file;
    Transformd orientation_transform = Transformd::Identity();
    Transformd extrinsic_transform = Transformd::Identity();
    std::array<float, 4> intrinsic_params{};
    std::array<float, 6> distortion_params{};
};

struct RieglScanPosition
{
    fs::path scan_file;
    Transformd transform = Transformd::Identity();
    std::vector<RieglImageFile> images;
};

class RieglProject
{
  public:
    /**
     * @brief Construct a new Riegl Project object
     *
     * @param dir
     * @param input_cloud_format  Implemented: rxp,ascii
     */
    RieglProject(std::string dir, std::string input_cloud_format = "rxp");
    bool parse_project(unsigned int start, unsigned int end);

    fs::path m_project_dir;
    std::vector<RieglScanPosition> m_scan_positions;
    std::string m_input_cloud_format;

  private:
    void parse_scanpositions(const detail::RieglXmlNode& project_ptree, unsigned int start, unsigned int end);
    void parse_images_per_scanpos(RieglScanPosition& scanpos,
                                  const detail::RieglXmlNode& scanpos_ptree,
                                  const detail::RieglXmlNode& project_ptree);
    void parse_asciiclouds();
};

std::ostream& operator<<(std::ostream& lhs, const RieglProject& rhs);
std::ostream& operator<<(std::ostream& lhs, const RieglScanPosition& rhs);

} // namespace lvr2
