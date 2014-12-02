#include "vec.h"
#include "mesh.h"
#include "mesh_gen.h"
#include "kernels.h"
#include "quadrature.h"
#include "util.h"
#include "petsc_interface.h"
#include "basis.h"
#include "bem.h"

int main() {
    // Fault mesh.
    auto fault = line_mesh({0, -1}, {0, 0}).refine_repeatedly(0);

    // Surface mesh
    auto surface = line_mesh({-4, 0}, {4, 0}).refine_repeatedly(9);

    // Quadrature details
    double far_threshold = 300.0;
    int near_quad_pts = 4;
    int near_steps = 8;
    int src_quad_pts = 2;
    int obs_quad_pts = 2;
    double tol = 1e-4;
    QuadStrategy<2> qs(obs_quad_pts, src_quad_pts, near_quad_pts,
                         near_steps, far_threshold, tol);

    std::vector<double> one_vec(2 * fault.facets.size(), 1.0);
    Problem<2> p = {fault, surface, laplace_double<2>, one_vec};

    auto disp = interpolate(surface, [&] (Vec2<double> x) {
            ObsPt<2> obs = {0.001, x, {0, 1}};
            double val = eval_integral_equation(p, qs, obs);
            return val;
        });

    hdf_out("antiplane.hdf5", surface, disp);
}
