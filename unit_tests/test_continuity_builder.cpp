#include "catch.hpp"
#include "continuity_builder.h"
#include "mesh_gen.h"

using namespace tbem;

Mesh<2> disjoint_mesh() {
    std::vector<Facet<2>> facets;
    for (int i = 0; i < 10; i++) {
        double val = 2 * i;
        facets.push_back({{
            {{val, -val}}, {{val + 1, -val - 1}}
        }});
    }
    return {facets};
}

Mesh<2> connected_mesh() {
    return line_mesh({{0, 0}}, {{0, 1}}).refine_repeatedly(2);
}

TEST_CASE("FindOverlappingVerticesFirstEmptyMesh", "[continuity_builder]") 
{
    Mesh<2> m{{}};
    auto overlaps = find_overlapping_vertices(m.begin(), m.begin());
}

TEST_CASE("FindOverlappingVerticesSecondEmptyMesh", "[continuity_builder]") 
{
    auto m1 = connected_mesh();
    Mesh<2> m2{{}};
    auto overlaps = find_overlapping_vertices(m1.begin(), m2.begin());
}

TEST_CASE("FindOverlappingVerticesDifferentMeshes", "[continuity_builder]") 
{
    auto m0 = disjoint_mesh();
    auto m1 = connected_mesh();
    auto overlaps = find_overlapping_vertices(m0.begin(), m1.begin());
    REQUIRE(overlaps.size() == 1);
    REQUIRE(overlaps.find(m0.begin())->second == m1.begin());
}

TEST_CASE("FindOverlappingVertices", "[continuity_builder]") 
{
    auto m = disjoint_mesh();
    auto overlaps = find_overlapping_vertices(m.begin(), m.begin());
    int n_verts = m.n_dofs();
    REQUIRE(overlaps.size() == n_verts);
    for (int i = 0; i < n_verts; i++) {
        auto v_it = m.begin() + i;
        REQUIRE(overlaps.find(v_it)->second == v_it);
    }
}

TEST_CASE("FindOverlappingVerticesSameMeshDisjoint", "[continuity_builder]") 
{
    auto m = disjoint_mesh();
    auto overlaps = find_overlapping_vertices_same_mesh(m.begin());
    REQUIRE(overlaps.size() == 0);
}

TEST_CASE("FindOverlappingVerticesSameMeshConnected", "[continuity_builder]") 
{
    auto m = connected_mesh();
    auto overlaps = find_overlapping_vertices_same_mesh(m.begin());
    REQUIRE(overlaps.size() == m.n_facets() - 1);

    int n_verts = m.n_dofs();
    for (int i = 1; i < n_verts - 2; i += 2) {
        auto v_it = m.begin() + i;
        REQUIRE(overlaps.find(v_it)->second == v_it + 1);
    }
}

TEST_CASE("ConstraintMesh", "[continuity_builder]") 
{
    auto sphere = sphere_mesh({{0, 0, 0}}, 1, 2);
    auto continuity = mesh_continuity(sphere.begin());
    auto constraints = convert_to_constraints(continuity);
    auto matrix = from_constraints(constraints);
    REQUIRE((3 * sphere.facets.size() == 384));
    REQUIRE(matrix.size() == 318);
    auto my_c = matrix.map.begin()->second.terms;
    REQUIRE(my_c[0].weight == 1);
}

TEST_CASE("RectMeshContinuity", "[continuity_builder]") 
{
    auto zplane = rect_mesh({{-1, -1, 0}}, {{1, -1, 0}}, {{1, 1, 0}}, {{-1, 1, 0}})
        .refine_repeatedly(1);
    auto continuity = mesh_continuity(zplane.begin());
    auto constraints = convert_to_constraints(continuity);
    REQUIRE(constraints.size() == 29);
}

TEST_CASE("CutWithDiscontinuity", "[continuity_builder]") 
{
    auto zplane = rect_mesh({{-1, -1, 0}}, {{1, -1, 0}}, {{1, 1, 0}}, {{-1, 1, 0}})
        .refine_repeatedly(1);
    auto xplane = rect_mesh({{0, -1, -1}}, {{0, 1, -1}}, {{0, 1, 1}}, {{0, -1, 1}})
        .refine_repeatedly(1);
    auto continuity = mesh_continuity(zplane.begin());
    auto cut_cont = cut_at_intersection(continuity, zplane.begin(), xplane.begin());
    auto constraints = convert_to_constraints(cut_cont);
    REQUIRE(constraints.size() == 16);
}

TEST_CASE("GetReducedToCountTheNumberOfVerticesOnASphereApproximation", "[continuity_builder]") 
{
    auto sphere = sphere_mesh({{0, 0, 0}}, 1, 0);
    auto continuity = mesh_continuity(sphere.begin());
    auto constraints = convert_to_constraints(continuity);
    auto matrix = from_constraints(constraints);
    std::vector<double> all(3 * sphere.facets.size(), 1.0);
    auto reduced = condense_vector(matrix, all);
    REQUIRE(reduced.size() == 6);
    REQUIRE_ARRAY_EQUAL(reduced, (std::vector<double>(6, 4.0)), 6);
}

TEST_CASE("ImposeNeighborBCs", "[continuity_builder]") 
{
    auto m0 = disjoint_mesh();
    auto m1 = connected_mesh();
    std::vector<double> bcs(m1.n_dofs(), 2.33);
    auto constraints = form_neighbor_bcs(m1.begin(), m0.begin(), bcs); 
    REQUIRE(constraints.size() == 1);
    REQUIRE(constraints[0].terms.size() == 1);
    REQUIRE(constraints[0].terms[0] == (LinearTerm{0, 1}));
    REQUIRE(constraints[0].rhs == 2.33);
}

TEST_CASE("InterpolateBCConstraints", "[continuity_builder]") {
    auto m0 = line_mesh({-1, 0}, {1, 0});
    auto cs = interpolate_bc_constraints<2>(m0, range(m0.n_dofs()),
        [](const Vec<double,2>& x) {
            return x[0] + 1.0;
        });
    REQUIRE(cs.size() == 2);
    REQUIRE(cs[0].terms[0] == (LinearTerm{0, 1}));
    REQUIRE(cs[0].rhs == 0.0);
    REQUIRE(cs[1].terms[0] == (LinearTerm{1, 1}));
    REQUIRE(cs[1].rhs == 2.0);
}

TEST_CASE("Normal constraints", "[continuity_builder]") 
{
    auto m0 = line_mesh({-1, -1}, {1, 1});
    auto cs = normal_constraints(m0, {0.0, 1.0});
    auto invsqrt2 = 1.0 / std::sqrt(2);
    REQUIRE(cs.size() == 2);
    REQUIRE(cs[0].terms.size() == 2);
    REQUIRE(cs[0].terms[0] == (LinearTerm{0, -invsqrt2}));
    REQUIRE(cs[1].terms[1] == (LinearTerm{3, invsqrt2}));
    REQUIRE(cs[1].rhs == 1.0);
}
