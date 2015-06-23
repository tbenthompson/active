#ifndef TBEMALNANSNNADNN_FACET_INFO_H
#define TBEMALNANSNNADNN_FACET_INFO_H

#include "mesh.h"
#include "vec.h"
#include "numerics.h"

namespace tbem {

template <size_t dim>
struct FacetInfo {
    const Facet<dim> facet;
    const double length_scale;
    const double jacobian;
    const Vec<double,dim> normal;

    static FacetInfo<dim> build(const Facet<dim>& facet);
};

template <size_t dim>
FacetInfo<dim> FacetInfo<dim>::build(const Facet<dim>& facet) {
    return FacetInfo<dim>{
        facet, facet_ball(facet).radius,
        facet_jacobian(facet), facet_normal(facet)
    };
}

template <size_t dim>
std::vector<FacetInfo<dim>> get_facet_info(const Mesh<dim>& m) {
    std::vector<FacetInfo<dim>> out;
    for (const auto& f: m.facets) {
        out.push_back(FacetInfo<dim>::build(f));
    }
    return out;
}

} //end namespace tbem

#endif
