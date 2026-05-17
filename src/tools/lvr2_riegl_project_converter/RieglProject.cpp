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

#include "RieglProject.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "lvr2/types/MatrixTypes.hpp"
#include "lvr2/geometry/BaseVector.hpp"
#include "lvr2/registration/TransformUtils.hpp"
namespace lvr2
{


namespace detail
{

namespace
{

std::string trim(std::string value)
{
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!value.empty() && is_space(static_cast<unsigned char>(value.front())))
    {
        value.erase(value.begin());
    }
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back())))
    {
        value.pop_back();
    }
    return value;
}

std::string decodeXmlEntities(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] != '&')
        {
            out.push_back(text[i]);
            continue;
        }

        const auto end = text.find(';', i + 1);
        if (end == std::string_view::npos)
        {
            out.push_back(text[i]);
            continue;
        }

        const auto entity = text.substr(i + 1, end - i - 1);
        if (entity == "amp") out.push_back('&');
        else if (entity == "lt") out.push_back('<');
        else if (entity == "gt") out.push_back('>');
        else if (entity == "quot") out.push_back('"');
        else if (entity == "apos") out.push_back('\'');
        else
        {
            out.append(text.substr(i, end - i + 1));
        }
        i = end;
    }
    return out;
}

} // namespace

class RieglXmlNode
{
  public:
    std::string name;
    std::string text;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<RieglXmlNode> children;

    [[nodiscard]] const RieglXmlNode& child_path(std::string_view path) const
    {
        const RieglXmlNode* current = this;
        std::size_t begin = 0;
        while (begin <= path.size())
        {
            const auto end = path.find('.', begin);
            const auto segment = path.substr(begin, end == std::string_view::npos ? path.size() - begin : end - begin);
            if (!segment.empty())
            {
                current = &current->child(segment);
            }
            if (end == std::string_view::npos)
            {
                break;
            }
            begin = end + 1;
        }
        return *current;
    }

    [[nodiscard]] const RieglXmlNode& child(std::string_view child_name) const
    {
        for (const auto& child_node : children)
        {
            if (child_node.name == child_name)
            {
                return child_node;
            }
        }
        throw std::runtime_error("missing XML child: " + std::string(child_name));
    }

    [[nodiscard]] std::string get_string(std::string_view path) const
    {
        constexpr std::string_view attr_prefix = "<xmlattr>.";
        constexpr std::string_view attr_segment = ".<xmlattr>.";

        if (path.starts_with(attr_prefix))
        {
            return attribute(path.substr(attr_prefix.size()));
        }

        const auto attr_pos = path.find(attr_segment);
        if (attr_pos != std::string_view::npos)
        {
            const auto node_path = path.substr(0, attr_pos);
            const auto attr_name = path.substr(attr_pos + attr_segment.size());
            return child_path(node_path).attribute(attr_name);
        }

        return trim(child_path(path).text);
    }

    [[nodiscard]] float get_float(std::string_view path) const
    {
        return std::stof(get_string(path));
    }

  private:
    [[nodiscard]] std::string attribute(std::string_view attr_name) const
    {
        const auto it = attributes.find(std::string(attr_name));
        if (it == attributes.end())
        {
            throw std::runtime_error("missing XML attribute: " + std::string(attr_name));
        }
        return it->second;
    }
};

class RieglXmlParser
{
  public:
    explicit RieglXmlParser(std::string input) : m_input(std::move(input)) {}

    [[nodiscard]] RieglXmlNode parse_document()
    {
        RieglXmlNode root;
        root.name = "#document";
        while (true)
        {
            skip_ws();
            if (m_pos >= m_input.size())
            {
                break;
            }
            if (m_input[m_pos] != '<')
            {
                ++m_pos;
                continue;
            }
            if (skip_markup())
            {
                continue;
            }
            root.children.push_back(parse_node());
        }
        return root;
    }

  private:
    [[nodiscard]] bool starts_with(std::string_view token) const
    {
        return std::string_view(m_input).substr(m_pos, token.size()) == token;
    }

    void skip_ws()
    {
        while (m_pos < m_input.size() && std::isspace(static_cast<unsigned char>(m_input[m_pos])) != 0)
        {
            ++m_pos;
        }
    }

    void expect(char expected)
    {
        if (m_pos >= m_input.size() || m_input[m_pos] != expected)
        {
            throw std::runtime_error(std::string("expected XML character '") + expected + "'");
        }
        ++m_pos;
    }

    [[nodiscard]] std::string read_name()
    {
        const auto begin = m_pos;
        while (m_pos < m_input.size())
        {
            const unsigned char c = static_cast<unsigned char>(m_input[m_pos]);
            if (std::isalnum(c) == 0 && m_input[m_pos] != '_' && m_input[m_pos] != '-' && m_input[m_pos] != ':' && m_input[m_pos] != '.')
            {
                break;
            }
            ++m_pos;
        }
        if (begin == m_pos)
        {
            throw std::runtime_error("expected XML name");
        }
        return m_input.substr(begin, m_pos - begin);
    }

    [[nodiscard]] std::string read_quoted_value()
    {
        skip_ws();
        const char quote = m_input.at(m_pos);
        if (quote != '\'' && quote != '"')
        {
            throw std::runtime_error("expected quoted XML attribute value");
        }
        ++m_pos;
        const auto begin = m_pos;
        const auto end = m_input.find(quote, begin);
        if (end == std::string::npos)
        {
            throw std::runtime_error("unterminated XML attribute value");
        }
        m_pos = end + 1;
        return decodeXmlEntities(std::string_view(m_input).substr(begin, end - begin));
    }

    bool skip_markup()
    {
        if (starts_with("<!--"))
        {
            skip_until("-->");
            return true;
        }
        if (starts_with("<?"))
        {
            skip_until("?>");
            return true;
        }
        if (starts_with("<![CDATA["))
        {
            return false;
        }
        if (starts_with("<!"))
        {
            skip_until(">");
            return true;
        }
        return false;
    }

    void skip_until(std::string_view token)
    {
        const auto end = m_input.find(std::string(token), m_pos + token.size());
        if (end == std::string::npos)
        {
            throw std::runtime_error("unterminated XML markup");
        }
        m_pos = end + token.size();
    }

    [[nodiscard]] RieglXmlNode parse_node()
    {
        expect('<');
        RieglXmlNode node;
        node.name = read_name();

        while (true)
        {
            skip_ws();
            if (starts_with("/>"))
            {
                m_pos += 2;
                return node;
            }
            if (m_pos < m_input.size() && m_input[m_pos] == '>')
            {
                ++m_pos;
                break;
            }
            const auto attr_name = read_name();
            skip_ws();
            expect('=');
            node.attributes[attr_name] = read_quoted_value();
        }

        while (m_pos < m_input.size())
        {
            if (starts_with("</"))
            {
                m_pos += 2;
                const auto close_name = read_name();
                if (close_name != node.name)
                {
                    throw std::runtime_error("mismatched XML closing tag: " + close_name);
                }
                skip_ws();
                expect('>');
                return node;
            }
            if (starts_with("<![CDATA["))
            {
                m_pos += 9;
                const auto end = m_input.find("]]>", m_pos);
                if (end == std::string::npos)
                {
                    throw std::runtime_error("unterminated XML CDATA");
                }
                node.text.append(m_input.substr(m_pos, end - m_pos));
                m_pos = end + 3;
                continue;
            }
            if (m_input[m_pos] == '<')
            {
                if (skip_markup())
                {
                    continue;
                }
                node.children.push_back(parse_node());
                continue;
            }

            const auto begin = m_pos;
            const auto end = m_input.find('<', begin);
            m_pos = end == std::string::npos ? m_input.size() : end;
            node.text.append(decodeXmlEntities(std::string_view(m_input).substr(begin, m_pos - begin)));
        }

        throw std::runtime_error("unterminated XML element: " + node.name);
    }

    std::string m_input;
    std::size_t m_pos = 0;
};

[[nodiscard]] RieglXmlNode readRieglXml(const fs::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("unable to open XML file: " + path.string());
    }
    std::string xml((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return RieglXmlParser(std::move(xml)).parse_document();
}

} // namespace detail

using Vec = BaseVector<float>;

// @TODO more errorprone against invalid input?
//       And this doesn't belong in here, it should be in Matrix4.
Transformd string2mat4f(const std::string data) 
{
    double mat[16];
    Transformd ret;

    int count = 0;
    std::string::size_type n = 0;
    std::string::size_type sz = 0;
    while (count < 16) {
        mat[count++] = std::stof(data.substr(n), &sz);
        n += sz;
        n = data.find_first_not_of(' ', n);
    }

    return buildTransformation(mat);
}

std::string get_first_group_regex(std::regex regex_string, std::string data) {
    std::regex regex(regex_string);
    std::smatch sm;
    std::regex_match(data, sm, regex);

    return sm.str(1);
}

RieglProject::RieglProject(
    std::string dir,
    std::string input_cloud_format
)
:m_project_dir(fs::path(dir))
,m_input_cloud_format(input_cloud_format)
{

}

void RieglProject::parse_scanpositions(const detail::RieglXmlNode& project_ptree, unsigned int start, unsigned int end) {
    int scan_id = 0;
    for (const auto& scanpos_info : project_ptree.child_path("project.scanpositions").children) {
        if (scanpos_info.name != "scanposition") continue;

        RieglScanPosition scanpos;

        std::string scan_name = scanpos_info.get_string("name");

        if (!scan_name.length() == 10) {
            std::cout << "[RieglProject] Warning: The scanpos " << scan_name << " is skipped"
                      << " because the name is malformed." << std::endl;

            continue;
        }

        unsigned int scan_pos_nr = std::stoul(scan_name.substr(7));

        // skip scanpositions that aren't in range
        if (scan_pos_nr < start || (end != 0 && scan_pos_nr > end)) {
            continue;
        }

        fs::path scan_dir = m_project_dir / ("SCANS/" + scan_name + "/");

        unsigned int scan_file_size = 0;

        // find the .rxp file for scanpos

        // @TODO What should happen in this case if it is possible?
        //       Currently we take the biggest file but maybe we should merge all scan files.
        for (const auto& scan_data : scanpos_info.child_path("singlescans").children) {
            if (scan_data.name != "scan") continue;

            std::string current_scan_filename = scan_data.get_string("file");

            // we don't want .mon files
            if (current_scan_filename.find(".mon") != std::string::npos) {
                continue;
            }

            fs::path current_scan_file = scan_dir / ("SINGLESCANS/" + current_scan_filename);

            if (!fs::exists(current_scan_file) || !fs::is_regular_file(current_scan_file)) {
                std::cout << "[RieglProject] Warning: The scan data file '" << current_scan_file
                          << "' doesn't exists or isn't a regular file and there \
                              is gonna be skipped." << std::endl;

                continue;
            }

            unsigned int current_scan_file_size = fs::file_size(current_scan_file);

            if ( current_scan_file_size > scan_file_size) {
                scanpos.scan_file = current_scan_file;
                scan_file_size = current_scan_file_size;
            }
        }

        // @TODO Maybe it should be possible to have scanpositions without scan data but
        //       with images. Then we would have to change this behaviour.
        if (scanpos.scan_file.empty()) {
            std::cout << "[RieglProject] Warning: The scanposition '" << scan_dir
                      << "' has no scan data and therefore we gonna skip this scanposition."
                      << std::endl;

            continue;
        }

        // parse scanpos transformation
        std::string transform = scanpos_info.get_string("sop.matrix");
        scanpos.transform = string2mat4f(transform);

        parse_images_per_scanpos(scanpos, scanpos_info, project_ptree);

        // @TODO maybe we shouldn't skip scanpositions only because they have no images.
        if (scanpos.images.empty()) {
            std::cout << "[RieglProject] Warning: Scanposition '" << scan_dir
                      << "' has no images and will be skipped." << std::endl;

            // continue;
        }

        if(scan_id < m_scan_positions.size())
        {
            m_scan_positions[scan_id] = scanpos;
        } else {
            m_scan_positions.push_back(scanpos);
        }

        scan_id++;
    }

}

void RieglProject::parse_images_per_scanpos(RieglScanPosition &scanpos,
                              const detail::RieglXmlNode& scanpos_ptree,
                              const detail::RieglXmlNode& project_ptree) {

    for (const auto& img_info : scanpos_ptree.child_path("scanposimages").children) {
        if (img_info.name != "scanposimage") continue;

        RieglImageFile img;

        std::string img_file = img_info.get_string("file");
        img.image_file = scanpos.scan_file.parent_path().parent_path() / ("SCANPOSIMAGES/" + img_file);


        if (!fs::exists(img.image_file) && !fs::is_regular_file(img.image_file)) {
            std::cout << "[RieglProject] Warning: Image file '" << img.image_file
                      << "' doesn't exists or is not a regular file and there is skipped."
                      << std::endl;

            continue;
        }



        // get orientation of image
        img.orientation_transform = string2mat4f(img_info.get_string("cop.matrix"));



        // @TODO refactor mountcalib and camcalib reference search into one...

        //get extrinsic transformation for image
        std::string mountcalib_ref = img_info.get_string("mountcalib_ref.<xmlattr>.noderef");
        mountcalib_ref = mountcalib_ref.substr(mountcalib_ref.find_last_of('/') + 1);

        bool found_mountcalib = false;
        for (const auto& mountcalib_info : project_ptree.child_path("project.calibrations.mountcalibs").children) {
            if (mountcalib_info.name != "mountcalib") continue;
            if (mountcalib_info.get_string("<xmlattr>.name") != mountcalib_ref) continue;

            found_mountcalib = true;

            img.extrinsic_transform = string2mat4f(mountcalib_info.get_string("matrix"));
        }

        // skip image if no calibration data was found...
        if (!found_mountcalib) {
            std::cout << "[RieglProject] Warning: Extrinsic transformation for image file '"
                      << img.image_file << "' wasn't found and image is gonna be skipped."
                      << std::endl;

            continue;
        }



        //get intrinsic params for image
        std::string camcalib_ref = img_info.get_string("camcalib_ref.<xmlattr>.noderef");
        camcalib_ref = camcalib_ref.substr(camcalib_ref.find_last_of('/') + 1);

        bool found_camcalib = false;
        for (const auto& camcalib_info : project_ptree.child_path("project.calibrations.camcalibs").children) {
            if (camcalib_info.name != "camcalib_opencv") continue;
            if (camcalib_info.get_string("<xmlattr>.name") != camcalib_ref) continue;

            found_camcalib = true;

            const auto& intrinsic_ptree = camcalib_info.child_path("internal_opencv");

            img.intrinsic_params[0] = intrinsic_ptree.get_float("fx");
            img.intrinsic_params[1] = intrinsic_ptree.get_float("fy");
            img.intrinsic_params[2] = intrinsic_ptree.get_float("cx");
            img.intrinsic_params[3] = intrinsic_ptree.get_float("cy");

            img.distortion_params[0] = intrinsic_ptree.get_float("k1");
            img.distortion_params[1] = intrinsic_ptree.get_float("k2");
            img.distortion_params[2] = intrinsic_ptree.get_float("k3");
            img.distortion_params[3] = intrinsic_ptree.get_float("k4");
            img.distortion_params[4] = intrinsic_ptree.get_float("p1");
            img.distortion_params[5] = intrinsic_ptree.get_float("p2");
        }

        // skip image if no calibration data was found...
        if (!found_camcalib) {
            std::cout << "[RieglProject] Warning: Camcalibration for image file '" << img.image_file
                      << "' wasn't found and image is gonna be skipped." << std::endl;

            continue;
        }

        scanpos.images.push_back(img);
    }
}

void RieglProject::parse_asciiclouds()
{
    
    fs::path scans_path = m_project_dir / "SCANS";
    
    if(!fs::exists(scans_path))
    {
        std::stringstream ss;
        ss << "[RieglProject] Error: RiSCAN scans path '" << scans_path
                << "' doesn't exist or isn't a directory";
        throw fs::filesystem_error(
            ss.str(),
            scans_path,
            std::make_error_code(std::errc::not_a_directory)
        );
    }

    std::vector<fs::path> path_vec;


    std::copy(
        fs::directory_iterator{scans_path},
        fs::directory_iterator{},
        std::back_inserter(path_vec)
    );

    // sort directories by name
    std::sort(path_vec.begin(), path_vec.end());

    for(int scan_id = 0; scan_id < path_vec.size(); scan_id ++)
    {
        fs::path cloud_path = path_vec[scan_id] / "POINTCLOUDS";

        if(!fs::exists(cloud_path))
        {
            std::stringstream ss;
            ss << "[RieglProject] Error: RiSCAN cloud path '" << cloud_path
                  << "' doesn't exist or isn't a directory";
            throw fs::filesystem_error(
                ss.str(),
                cloud_path,
                std::make_error_code(std::errc::not_a_directory)
            );
        }

        std::vector<fs::path> potential_clouds;

        std::copy(
            fs::directory_iterator{cloud_path},
            fs::directory_iterator{},
            std::back_inserter(potential_clouds)
        );

        std::sort(
            potential_clouds.begin(),
            potential_clouds.end(),
            [](const fs::path& a, const fs::path& b) {
                return fs::file_size(a) > fs::file_size(b);
            }
        );

        fs::path biggest_cloud_path = potential_clouds[0];

        if(scan_id < m_scan_positions.size())
        {
            m_scan_positions[scan_id].scan_file = biggest_cloud_path;
        } else {
            RieglScanPosition sp;
            sp.scan_file = biggest_cloud_path;
            m_scan_positions.push_back(sp);
        }

    }
}

bool RieglProject::parse_project(unsigned int start, unsigned int end) {
    // check if project path exists
    if (!fs::exists(m_project_dir) || !fs::is_directory(m_project_dir)) {
        std::cout << "[RieglProject] Error: RiSCAN project path '" << m_project_dir
                  << "' doesn't exist or isn't a directory" << std::endl;

        return false;
    }

    // check if project.rsp exists
    fs::path project_file = m_project_dir / "project.rsp";
    if (!fs::exists(project_file) || !fs::is_regular_file(project_file)) {
        std::cout << "[RieglProject] Error: The RiSCAN project file '" << project_file
                  << "' doesn't exist or isn't a file." << std::endl;

        return false;
    }

    // read project.rsp file
    detail::RieglXmlNode project_ptree = detail::readRieglXml(project_file);

    parse_scanpositions(project_ptree, start, end);

    if (m_scan_positions.empty()) {
        std::cout << "[RieglProject] Error: Unable to parse any scanposition." << std::endl;

        return false;
    }

    // search for ascii pointcloud files if input cloud format is specified as "ascii"
    if(m_input_cloud_format == "ascii")
    {
        parse_asciiclouds();
    }

    return true;
}

std::ostream& operator<<(std::ostream &lhs, const RieglProject &rhs) {
    lhs << "Scan Project dir: " << rhs.m_project_dir << "\n";

    for (const lvr2::RieglScanPosition &sp : rhs.m_scan_positions) {
        lhs << "\n" << sp;
    }

    return lhs;
}

std::ostream& operator<<(std::ostream &lhs, const RieglScanPosition &rhs) {
    lhs << "Scan File: " << rhs.scan_file << " "
        << fs::file_size(rhs.scan_file) << "\n"
        << rhs.transform
        << "Images: ";

    for (RieglImageFile img : rhs.images) {
        lhs << "\n\t" << img.image_file.filename() << " " << fs::file_size(img.image_file) << '\n'
            << "Orientation: " << img.orientation_transform << '\n'
            << "Extrinsic: " << img.extrinsic_transform << '\n';

        lhs << "Intrinsic: ";
        for (int i = 0; i < 4; i++) {
            lhs << img.intrinsic_params[i] << " ";
        }

        lhs << '\n' << "Distortion: ";
        for (int i = 0; i < 6; i++) {
            lhs << img.distortion_params[i] << " ";
        }
    }

    lhs << std::endl;

    return lhs;
}

} // namespace lvr2
