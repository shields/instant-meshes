/*
    Copyright © 2026 Michael Shields

    Use of this source code is governed by a BSD-style license that can be found
    in the LICENSE.txt file.
*/

#include "meshio.h"
#include "dedge.h"

#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace {

using Point = std::array<Float, 3>;
using Triangle = std::array<Point, 3>;
const Triangle triangle {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}};

void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}

void append_u32(std::string &bytes, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes.push_back(char((value >> shift) & 255));
}

std::string binary(const std::vector<Triangle> &triangles,
                   const std::string &header = "STL fixture",
                   const Point &normal = {0, 0, 1}) {
    std::string bytes = header;
    bytes.resize(80, '\0');
    append_u32(bytes, uint32_t(triangles.size()));
    for (const auto &facet : triangles) {
        for (Float value : normal)
            append_u32(bytes, std::bit_cast<uint32_t>(float(value)));
        for (const auto &point : facet)
            for (Float value : point)
                append_u32(bytes, std::bit_cast<uint32_t>(float(value)));
        bytes.append("\x34\x12", 2);
    }
    return bytes;
}

std::string ascii(const std::vector<Triangle> &triangles) {
    std::ostringstream text;
    text << "solid fixture\n";
    for (const auto &facet : triangles) {
        text << "facet normal 0 0 1\nouter loop\n";
        for (const auto &point : facet)
            text << "vertex " << point[0] << ' ' << point[1] << ' ' << point[2] << '\n';
        text << "endloop\nendfacet\n";
    }
    text << "endsolid fixture\n";
    return text.str();
}

std::pair<MatrixXu, MatrixXf> parse(const std::string &bytes,
                                 const ProgressCallback &progress = {}) {
    std::istringstream input(bytes);
    MatrixXu F;
    MatrixXf V;
    read_stl(input, F, V, progress);
    return {F, V};
}

void reject_stream(std::istream &input, std::string_view message,
                   const ProgressCallback &progress = {}) {
    MatrixXu F = MatrixXu::Constant(3, 1, 42);
    MatrixXf V = MatrixXf::Constant(3, 1, 42);
    bool rejected = false;
    try {
        read_stl(input, F, V, progress);
    } catch (const std::runtime_error &error) {
        require(std::string_view(error.what()).find(message) != std::string_view::npos,
                "Wrong error: " + std::string(error.what()));
        rejected = true;
    }
    require(rejected, "Invalid STL was accepted: " + std::string(message));
    require(F == MatrixXu::Constant(3, 1, 42) && V == MatrixXf::Constant(3, 1, 42),
            "Failed import modified its output");
}

void reject(const std::string &bytes, std::string_view message) {
    std::istringstream input(bytes);
    reject_stream(input, message);
}

std::string replace(const std::string &text, const std::string &before,
                    const std::string &after) {
    auto result = text;
    auto position = result.find(before);
    require(position != std::string::npos, "Fixture token not found");
    result.replace(position, before.size(), after);
    return result;
}

void test_geometry() {
    const Triangle adjacent {{{-0.0f, 1, -0.0f}, {1, 0, 0}, {1, 1, 0}}};
    MatrixXu expectedF(3, 2);
    expectedF << 0, 2, 1, 1, 2, 3;
    MatrixXf expectedV(3, 4);
    expectedV << 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0;
    for (const auto &bytes : {ascii({triangle, adjacent}),
                              binary({triangle, adjacent}),
                              binary({triangle, adjacent}, "solid fixture\nfacet normal misleading header")}) {
        auto [F, V] = parse(bytes);
        require(F == expectedF && V == expectedV, "STL changes coordinates, winding, or shared vertices");
        VectorXu V2E, E2E;
        VectorXb boundary, nonmanifold;
        build_dedge(F, V, V2E, E2E, boundary, nonmanifold, {}, true);
        require(E2E[1] == 3 && E2E[3] == 1, "Imported facets do not share their common edge");
    }
    Triangle close = triangle;
    close[0][0] = 1e-10f;
    auto [F, V] = parse(binary({triangle, close}));
    require(V.cols() == 4 && F(0, 1) != F(0, 0), "STL welds unequal coordinates");

    auto named = ascii({triangle});
    named = replace(named, "solid fixture", "solid name with spaces and Unicode \xc3\xa9");
    named = replace(named, "endsolid fixture\n", "endsolid");
    require(parse(named).first.cols() == 1, "ASCII solid names or missing final newline fail");
    require(parse(ascii({triangle}) + ascii({triangle})).first.cols() == 2,
            "Multiple ASCII solids fail");
    require(parse(replace(ascii({triangle}), "solid fixture", "solid")).first.cols() == 1,
            "Unnamed ASCII solid fails");
    auto scientific = replace(ascii({triangle}), "vertex 1 0 0", "vertex +1e0 -0.0 +0");
    require(parse(scientific).second.col(1) == Vector3f(1, 0, 0), "Signed scientific numbers fail");
    std::string crlf;
    for (char c : scientific) {
        if (c == '\n')
            crlf += '\r';
        crlf += c;
    }
    crlf = " \t\r\n\f\v" + crlf + "\t\r\n";
    require(parse(crlf).first.cols() == 1, "ASCII whitespace handling fails");
}

void test_errors() {
    const auto valid = ascii({triangle});
    reject("", "no triangles");
    reject(" \t\r\n", "no triangles");
    reject(ascii({}), "no triangles");
    reject(binary({}), "no triangles");
    reject("solid", "Unexpected end");
    reject(valid + "garbage", "expected solid");
    reject(replace(valid, "facet normal", "nonsense normal"), "expected facet or endsolid");
    for (const auto &token : {"normal", "outer", "loop", "vertex", "endloop", "endfacet"})
        reject(replace(valid, token, "invalid"), "expected " + std::string(token));
    for (const auto &token : {"normal", "outer", "loop", "vertex", "endloop", "endfacet", "endsolid"}) {
        auto position = valid.find(token);
        reject(valid.substr(0, position), "Unexpected end");
    }
    for (const auto &number : {"invalid", "1suffix", "+", "-", "++1", "+-1", "-+1", "1e100", "1e-1000"})
        reject(replace(valid, "vertex 1 0 0", "vertex " + std::string(number) + " 0 0"), "invalid number");
    for (const auto &number : {"nan", "inf", "-inf", "+nan"}) {
        reject(replace(valid, "vertex 1 0 0", "vertex " + std::string(number) + " 0 0"), "non-finite");
        reject(replace(valid, "normal 0", "normal " + std::string(number)), "non-finite");
    }
    for (Float value : {std::numeric_limits<Float>::quiet_NaN(), std::numeric_limits<Float>::infinity(),
                        -std::numeric_limits<Float>::infinity()}) {
        auto corrupt = triangle;
        corrupt[0][0] = value;
        reject(binary({corrupt}), "non-finite");
        reject(binary({triangle}, "fixture", {value, 0, 0}), "non-finite");
    }
    for (const Triangle &bad : std::vector<Triangle> {
            {{{0, 0, 0}, {0, 0, 0}, {0, 1, 0}}},
            {{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}}}) {
        reject(ascii({bad}), "degenerate triangle");
        reject(binary({bad}), "degenerate triangle");
    }
    auto extra = replace(valid, "endloop", "vertex 0 0 1\nendloop");
    reject(extra, "expected endloop");
    auto bytes = binary({triangle});
    for (size_t length : {size_t(1), size_t(79), size_t(80), size_t(83), size_t(84), size_t(133)})
        reject(bytes.substr(0, length), "expected solid");
    reject(bytes + "trailing bytes", "expected solid");
    bytes[80] = 2;
    reject(bytes, "expected solid");
    for (Float abort_at : {Float(0), Float(1)}) {
        std::istringstream input(valid);
        reject_stream(input, "cancelled", [=](const std::string &, Float value) {
            if (value == abort_at)
                throw std::runtime_error("cancelled");
        });
    }
}

void test_io_errors() {
    std::istream unavailable(nullptr);
    reject_stream(unavailable, "determine STL input size");
    struct HugeBuffer : std::streambuf {
        pos_type seekoff(off_type, std::ios::seekdir, std::ios::openmode) override {
            return pos_type(85 + 50 * (uint64_t(INVALID) / 3));
        }
    } huge;
    std::istream oversized(&huge);
    reject_stream(oversized, "index capacity");
    struct ShortBuffer : std::stringbuf {
        using std::stringbuf::stringbuf;
        std::streamsize xsgetn(char *, std::streamsize) override { return 0; }
    } short_buffer(ascii({triangle}));
    std::istream unreadable(&short_buffer);
    reject_stream(unreadable, "read STL data");
    MatrixXu F;
    MatrixXf V;
    bool rejected = false;
    try {
        load_stl("this-directory-does-not-exist/mesh.stl", F, V);
    } catch (const std::runtime_error &error) {
        require(std::string_view(error.what()).find("Unable to open STL") != std::string_view::npos,
                "Missing STL gives wrong error");
        rejected = true;
    }
    require(rejected, "Missing STL was accepted");
}

void test_progress() {
    const std::vector<Triangle> facets(2050, triangle);
    for (const auto &bytes : {ascii(facets), binary(facets)}) {
        std::vector<Float> values;
        auto [F, V] = parse(bytes, [&](const std::string &message, Float value) {
            require(message == "Loading STL", "Unexpected progress message");
            require(std::isfinite(value) && value >= 0 && value <= 1, "Invalid import progress");
            values.push_back(value);
        });
        require(F.cols() == 2050 && V.cols() == 3, "Large STL import loses geometry");
        require(values.size() == 4 && values.front() == 0 && values.back() == 1 &&
                std::is_sorted(values.begin(), values.end()), "STL progress is missing or runs backwards");
        require(parse(bytes).first == F, "Progress callback changes STL import");
    }
}

void write(const std::filesystem::path &path, const std::string &bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(bytes.data(), std::streamsize(bytes.size()));
    require(bool(file), "Cannot write fixture " + path.string());
}

void test_dispatch(const std::filesystem::path &directory) {
    require(mesh_input_file_types() == std::vector<std::pair<std::string, std::string>> {
        {"obj", "Wavefront OBJ"}, {"ply", "Stanford PLY"}, {"aln", "Aligned point cloud"},
        {"stl", "Stereolithography STL"}}, "GUI file types do not include the supported input formats");
    for (const auto &[extension, description] : mesh_input_file_types()) {
        auto name = "mesh." + extension;
        require(mesh_input_filename(name) == name, "GUI modifies a supported filename");
    }
    for (const auto &name : {"mesh.STL", "mesh.PLY", "mesh.OBJ", "mesh.ALN"})
        require(mesh_input_filename(name) == name, "GUI rejects an uppercase extension");
    for (const auto &name : {"mesh", "mesh.other", "", "a"})
        require(mesh_input_filename(name) == std::string(name) + ".ply", "GUI default extension changes");

    write(directory / "triangle.stl", ascii({triangle}));
    write(directory / "triangle.STL", binary({triangle}));
    write(directory / "triangle.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    const std::string ply_header = "ply\nformat ascii 1.0\nelement vertex 3\nproperty float x\n"
        "property float y\nproperty float z\n";
    write(directory / "triangle.ply", ply_header + "element face 1\nproperty list uchar int vertex_indices\n"
          "end_header\n0 0 0\n1 0 0\n0 1 0\n3 0 1 2\n");
    write(directory / "points.ply", ply_header + "property float nx\nproperty float ny\nproperty float nz\n"
          "end_header\n0 0 0 0 0 1\n1 0 0 0 0 1\n0 1 0 0 0 1\n");
    write(directory / "points.aln", "1\npoints.ply\n1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n");
    for (const auto &name : {"triangle.stl", "triangle.STL", "triangle.obj", "triangle.ply", "points.aln"}) {
        MatrixXu F;
        MatrixXf V, N;
        load_mesh_or_pointcloud((directory / name).string(), F, V, N);
        require(V.cols() == 3, "Mesh dispatch changes vertex count");
        if (std::string_view(name) == "points.aln")
            require(F.size() == 0 && N.cols() == 3, "ALN dispatch changes");
        else
            require(F.cols() == 1, "Mesh dispatch loses triangles");
    }
    for (const auto &name : {"x", "mesh.unknown"}) {
        MatrixXu F;
        MatrixXf V, N;
        bool rejected = false;
        try {
            load_mesh_or_pointcloud(name, F, V, N);
        } catch (const std::runtime_error &error) {
            require(std::string_view(error.what()).find("Unknown file extension") != std::string_view::npos,
                    "Unknown format gives wrong error");
            rejected = true;
        }
        require(rejected, "Unknown input format accepted");
    }

    const std::array<Point, 6> vertices {{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0},
                                       {0, 0, 1}, {0, 0, -1}}};
    const std::array<std::array<size_t, 3>, 8> faces {{{0, 2, 4}, {2, 1, 4}, {1, 3, 4}, {3, 0, 4},
                                                    {2, 0, 5}, {1, 2, 5}, {3, 1, 5}, {0, 3, 5}}};
    std::vector<Triangle> octahedron;
    for (const auto &face : faces)
        octahedron.push_back({vertices[face[0]], vertices[face[1]], vertices[face[2]]});
    write(directory / "octahedron-ascii.stl", ascii(octahedron));
    write(directory / "octahedron-binary.STL", binary(octahedron, "solid octahedron\nfacet normal header"));
    write(directory / "invalid.stl", replace(ascii({triangle}), "vertex 1", "vertex invalid"));
}

}

int main(int argc, char **argv) {
    try {
        require(argc == 2, "Expected a fixture directory");
        std::filesystem::path directory(argv[1]);
        std::filesystem::create_directories(directory);
        test_geometry();
        test_errors();
        test_io_errors();
        test_progress();
        test_dispatch(directory);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
