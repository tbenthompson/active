#ifndef __TEST_SHARED_H
#define __TEST_SHARED_H

#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <string>
#include "vec.h"

namespace tbem {

#define TIC\
    std::chrono::high_resolution_clock::time_point start =\
        std::chrono::high_resolution_clock::now();\
    int time_ms;
#define TIC2\
    start = std::chrono::high_resolution_clock::now();
#define TOC(name)\
    time_ms = std::chrono::duration_cast<std::chrono::milliseconds>\
                (std::chrono::high_resolution_clock::now() - start).count();\
    std::cout << name << " took "\
              << time_ms\
              << "ms.\n";

std::vector<int> naturals(int min, int max);
std::vector<int> naturals(int max);

inline std::array<std::vector<double>,3> three_pts() {
    std::array<std::vector<double>,3> es;
    es[0] = {1.0, -1.0, 0.0};
    es[1] = {2.0, 0.0, -2.0};
    es[2] = {0.0, -3.0, 3.0};
    return es;
}

inline std::vector<double> random_list(int N) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    std::vector<double> es(N);
    for (int i = 0; i < N; ++i) {
        es[i] = dis(gen);
    }
    return es;
}

inline std::array<std::vector<double>,3> random_pts(int N) {
    std::array<std::vector<double>,3> locs = 
        {random_list(N), random_list(N), random_list(N)};
    return locs;
}

inline double random_val() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    return dis(gen);
}

inline Vec3<double> random_pt() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    return {
        dis(gen), dis(gen), dis(gen)
    };
}

inline Vec3<double> spherify_pt(Vec3<double> pt, 
                                        Vec3<double> c, double r) {
    double rf = pt[0];
    double tf = pt[1] * 2 * M_PI;
    double pf = pt[2] * M_PI;
    Vec3<double> ret = {
        c[0] + r * rf * std::cos(tf) * std::sin(pf),
        c[1] + r * rf * std::sin(tf) * std::sin(pf),
        c[2] + r * rf * std::cos(pf)
    };
    return ret;
}

template <int dim>
inline Vec<double,dim> random_pt_sphere(Vec<double,dim> c, double r);

template <>
inline Vec3<double> random_pt_sphere<3>(Vec3<double> c, double r) {
    auto pt = random_pt();
    return spherify_pt(pt, c, r);
}

template <>
inline Vec2<double> random_pt_sphere<2>(Vec2<double> c, double r) {
    auto sphere_pt = random_pt_sphere<3>({c[0], c[1], 0.0}, r);
    return {sphere_pt[0], sphere_pt[1]};
}

inline double error_inf(const std::vector<double>& a, 
                        const std::vector<double>& b) {
    double error = 0.0;
    for (unsigned int i = 0; i < a.size(); i++) {
        error = std::max(std::fabs(a[i] - b[i]), error);
    }
    return error;
}

//Forward declaration of Mesh.
template <typename T, int dim>
struct MeshField;
template <int dim>
using Mesh = MeshField<Vec<double,dim>,dim>;

template <int dim>
void hdf_out_surface(const std::string& filename, const Mesh<dim>& mesh,
                     const std::vector<std::vector<double>>& data);

template <int dim> 
void hdf_out_volume(const std::string& filename,
                    const std::vector<Vec<double,dim>>& points,
                    const std::vector<std::vector<double>>& data);

} //END namespace tbem
#endif