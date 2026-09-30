/*
    Copyright © 2026 Michael Shields

    Use of this source code is governed by a BSD-style license that can be found
    in the LICENSE.txt file.
*/

#include "meshio.h"

#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <string_view>

namespace {

constexpr std::string_view whitespace = " \t\r\n\f\v";
constexpr uint64_t maximum_size = 84 + 50 * (uint64_t(std::numeric_limits<uint32_t>::max()) / 3);

uint32_t little_endian_u32(const char *bytes) {
    const auto *p = reinterpret_cast<const unsigned char *>(bytes);
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

std::array<Float, 3> binary_vector(const char *bytes) {
    std::array<Float, 3> result;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = std::bit_cast<float>(little_endian_u32(bytes + 4 * i));
        if (!std::isfinite(result[i]))
            throw std::runtime_error("STL contains a non-finite number");
    }
    return result;
}

struct AsciiReader {
    std::string_view remaining;

    bool finished() const {
        return remaining.find_first_not_of(whitespace) == std::string_view::npos;
    }

    std::string_view word() {
        size_t start = remaining.find_first_not_of(whitespace);
        if (start == std::string_view::npos)
            throw std::runtime_error("Unexpected end of ASCII STL");
        remaining.remove_prefix(start);
        auto result = remaining.substr(0, remaining.find_first_of(whitespace));
        remaining.remove_prefix(result.size());
        return result;
    }

    void expect(std::string_view expected) {
        if (word() != expected)
            throw std::runtime_error("ASCII STL: expected " + std::string(expected));
    }

    void skip_name() {
        size_t end = remaining.find('\n');
        if (end == std::string_view::npos)
            remaining = {};
        else
            remaining.remove_prefix(end + 1);
    }

    std::array<Float, 3> vector() {
        std::array<Float, 3> result;
        for (Float &value : result) {
            auto token = word();
            if (token.front() == '+') {
                token.remove_prefix(1);
                if (token.starts_with('-'))
                    throw std::runtime_error("ASCII STL contains an invalid number");
            }
            auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
            if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size())
                throw std::runtime_error("ASCII STL contains an invalid number");
            if (!std::isfinite(value))
                throw std::runtime_error("STL contains a non-finite number");
        }
        return result;
    }
};

struct MeshBuilder {
    std::vector<std::array<Float, 3>> vertices;
    std::vector<std::array<uint32_t, 3>> faces;
    std::map<std::array<Float, 3>, uint32_t> indices;

    void triangle(const std::array<std::array<Float, 3>, 3> &points) {
        Eigen::Vector3d a = Eigen::Map<const Vector3f>(points[0].data()).cast<double>();
        Eigen::Vector3d b = Eigen::Map<const Vector3f>(points[1].data()).cast<double>();
        Eigen::Vector3d c = Eigen::Map<const Vector3f>(points[2].data()).cast<double>();
        if ((b - a).cross(c - a).squaredNorm() == 0)
            throw std::runtime_error("STL contains a degenerate triangle");
        std::array<uint32_t, 3> face;
        for (size_t i = 0; i < points.size(); ++i) {
            auto [entry, inserted] = indices.try_emplace(points[i], uint32_t(vertices.size()));
            if (inserted)
                vertices.push_back(points[i]);
            face[i] = entry->second;
        }
        faces.push_back(face);
    }

    void finish(MatrixXu &F, MatrixXf &V, const ProgressCallback &progress) const {
        if (faces.empty())
            throw std::runtime_error("STL contains no triangles");
        MatrixXu resultF(3, faces.size());
        MatrixXf resultV(3, vertices.size());
        for (size_t i = 0; i < faces.size(); ++i)
            resultF.col(i) = Eigen::Map<const Vector3u>(faces[i].data());
        for (size_t i = 0; i < vertices.size(); ++i)
            resultV.col(i) = Eigen::Map<const Vector3f>(vertices[i].data());
        if (progress)
            progress("Loading STL", 1);
        F = std::move(resultF);
        V = std::move(resultV);
    }
};

}

void read_stl(std::istream &input, MatrixXu &F, MatrixXf &V,
              const ProgressCallback &progress) {
    input.seekg(0, std::ios::end);
    std::streamoff size = input.tellg();
    if (size < 0)
        throw std::runtime_error("Unable to determine STL input size");
    if (uint64_t(size) > maximum_size)
        throw std::runtime_error("STL exceeds 32-bit mesh index capacity");
    input.seekg(0);
    if (progress)
        progress("Loading STL", 0);
    std::string data(size_t(size), '\0');
    if (!input.read(data.data(), size))
        throw std::runtime_error("Unable to read STL data");

    MeshBuilder mesh;
    uint32_t count = 0;
    if (data.size() >= 84)
        count = little_endian_u32(data.data() + 80);
    if (data.size() >= 84 && data.size() == 84 + 50 * uint64_t(count)) {
        for (uint32_t i = 0; i < count; ++i) {
            const char *facet = data.data() + 84 + 50 * size_t(i);
            binary_vector(facet);
            mesh.triangle({binary_vector(facet + 12), binary_vector(facet + 24),
                           binary_vector(facet + 36)});
            if (progress && (i + 1) % 1024 == 0)
                progress("Loading STL", Float(i + 1) / count);
        }
    } else {
        AsciiReader ascii {data};
        while (!ascii.finished()) {
            ascii.expect("solid");
            ascii.skip_name();
            while (true) {
                auto token = ascii.word();
                if (token == "endsolid") {
                    ascii.skip_name();
                    break;
                }
                if (token != "facet")
                    throw std::runtime_error("ASCII STL: expected facet or endsolid");
                ascii.expect("normal");
                ascii.vector();
                ascii.expect("outer");
                ascii.expect("loop");
                std::array<std::array<Float, 3>, 3> points;
                for (auto &point : points) {
                    ascii.expect("vertex");
                    point = ascii.vector();
                }
                ascii.expect("endloop");
                ascii.expect("endfacet");
                mesh.triangle(points);
                if (progress && mesh.faces.size() % 1024 == 0)
                    progress("Loading STL", Float(data.size() - ascii.remaining.size()) / size);
            }
        }
    }
    mesh.finish(F, V, progress);
}

void load_stl(const std::string &filename, MatrixXu &F, MatrixXf &V,
              const ProgressCallback &progress) {
    std::ifstream input(filename, std::ios::binary);
    if (!input)
        throw std::runtime_error("Unable to open STL file \"" + filename + "\"");
    Timer<> timer;
    cout << "Loading \"" << filename << "\" .. " << std::flush;
    read_stl(input, F, V, progress);
    cout << "done. (V=" << V.cols() << ", F=" << F.cols()
         << ", took " << timeString(timer.value()) << ")" << endl;
}
