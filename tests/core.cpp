/*
    Copyright © 2026 Michael Shields

    Use of this source code is governed by a BSD-style license that can be found
    in the LICENSE.txt file.
*/

#include "bvh.h"
#include "parallel_stable_sort.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <utility>

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

void test_sort() {
    std::mt19937 random(1234);
    for (int count : {0, 1, 2048, 2049, 10000}) {
        std::vector<std::pair<int, int>> values;
        for (int i = 0; i < count; ++i)
            values.emplace_back(random() % 13, i);
        auto expected = values;
        auto compare = [](const auto &a, const auto &b) { return a.first < b.first; };
        std::stable_sort(expected.begin(), expected.end(), compare);
        parallel_stable_sort(values.begin(), values.end(), compare);
        require(values == expected, "Parallel sort loses order or stability");
    }
}

void test_triangles() {
    constexpr int count = 128;
    MatrixXf vertices(3, 3 * count), normals;
    MatrixXu faces(3, count);
    AABB bounds;
    for (int i = 0; i < count; ++i) {
        vertices.col(3 * i) = Vector3f(2 * i, 0, 0);
        vertices.col(3 * i + 1) = Vector3f(2 * i + 1, 0, 0);
        vertices.col(3 * i + 2) = Vector3f(2 * i, 1, 0);
        faces.col(i) = Vector3u(3 * i, 3 * i + 1, 3 * i + 2);
        for (int j = 0; j < 3; ++j)
            bounds.expandBy(vertices.col(3 * i + j));
    }
    BVH bvh(&faces, &vertices, &normals, bounds);
    bvh.build();
    for (int i = 0; i < count; ++i) {
        uint32_t index = count;
        Float distance = 0;
        Ray ray(Vector3f(2 * i + .25f, .25f, 1), Vector3f(0, 0, -1));
        require(bvh.rayIntersect(ray, index, distance), "BVH misses a triangle");
        require(index == static_cast<uint32_t>(i), "BVH returns the wrong triangle");
        require(std::abs(distance - 1) < 1e-6f, "BVH returns the wrong distance");
        ray.maxt = .5f;
        require(!bvh.rayIntersect(ray), "BVH ignores ray length");
    }
    require(!bvh.rayIntersect(Ray(Vector3f(-1, -1, 1), Vector3f(0, 0, -1))),
            "BVH reports a hit outside the mesh");
}

void test_points() {
    constexpr int side = 12;
    MatrixXf vertices(3, side * side), normals(3, side * side);
    MatrixXu faces;
    AABB bounds;
    for (int y = 0; y < side; ++y) {
        for (int x = 0; x < side; ++x) {
            int i = y * side + x;
            vertices.col(i) = Vector3f(.01f * x, .01f * y, 0);
            normals.col(i) = Vector3f(0, 0, 1);
            bounds.expandBy(vertices.col(i));
        }
    }
    BVH bvh(&faces, &vertices, &normals, bounds);
    bvh.build();
    require(std::abs(bvh.diskRadius() - .03f) < 1e-6f, "Point cloud disk radius loses fractional distances");
    for (int i = 0; i < vertices.cols(); ++i) {
        Float radius = std::numeric_limits<Float>::infinity();
        uint32_t nearest = bvh.findNearest(vertices.col(i), radius);
        require(nearest < static_cast<uint32_t>(vertices.cols()) && nearest != static_cast<uint32_t>(i),
                "BVH returns an invalid nearest neighbor");
        if (std::abs(radius - .01f) >= 1e-6f)
            std::cerr << "Point " << i << ": neighbor " << nearest << ", distance " << radius << std::endl;
        require(std::abs(radius - .01f) < 1e-6f, "BVH returns the wrong neighbor distance");
    }
}

int main() {
    try {
        for (int threads : {1, 4}) {
            tbb::global_control concurrency(tbb::global_control::max_allowed_parallelism, threads);
            test_sort();
            test_triangles();
            test_points();
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
    return 0;
}
