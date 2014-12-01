#ifndef __HJKHSDFLJSLLHA_LAPLACE_H
#define __HJKHSDFLJSLLHA_LAPLACE_H
#include <functional>
#include "vec.h"
#include "mesh.h"
#include "bem.h"
#include "kernels.h"
#include "quadrature.h"
#include "util.h"
#include "petsc_interface.h"

template <int dim>
using HarmonicFnc = std::function<double(Vec<double,dim>)>;

template <int dim>
using HarmonicDeriv = std::function<double(Vec<double,dim>, Vec<double,dim>)>;

template <int dim>
void run_laplace_test(const Mesh<dim>& mesh,
                      std::vector<Vec<double,dim>> test_interior_pts,
                      const HarmonicFnc<dim>& fnc,
                      const HarmonicDeriv<dim>& deriv) {
    double far_threshold = 3.0;
    int near_quad_pts = 3;
    int near_steps = 8;
    int src_quad_pts = 2;
    //TODO: Something is seriously wrong when I use obs_quad_pts = 3
    int obs_quad_pts = 2;
    double tol = 1e-4;
    double linear_solve_tol = 1e-5;

    QuadStrategy<dim> qs(obs_quad_pts, src_quad_pts, near_quad_pts,
                         near_steps, far_threshold, tol);

    auto constraints = ConstraintMatrix::from_constraints(mesh_continuity(mesh));

    //TODO: Interpolate function
    int n_dofs = dim * mesh.facets.size();
    std::vector<double> u(n_dofs);
    for (unsigned int i = 0; i < mesh.facets.size(); i++) {
        for (int d = 0; d < dim; d++) {
            u[dim * i + d] = fnc(mesh.facets[i].vertices[d]);
        }
    }

    std::vector<double> dudn(n_dofs);
    for (unsigned int i = 0; i < mesh.facets.size(); i++) {
        for (int d = 0; d < dim; d++) {
            auto n = normalized(unscaled_normal(mesh.facets[i].vertices));
            dudn[dim * i + d] = deriv(mesh.facets[i].vertices[d], n); 
        }
    }

    // Construct and evaluate the RHS for a Dirichlet Laplace problem:
    // The integral equation is: DoubleLayer(u) + u = SingleLayer(dudn)
    // First, the double layer potential is evaluated (DoubleLayer(u))
    // This is added to the mass matrix term (u)
    TIC
    Problem<dim> p_double = {mesh, mesh, laplace_double<dim>, u};
    auto rhs_double = direct_interact(p_double, qs);

    Problem<dim> p_mass = {mesh, mesh, one<dim>, u};
    auto rhs_mass = mass_term(p_mass, qs);
    
    std::vector<double> rhs_full(n_dofs);
    double mass_factor[2] = {0.75, 1.0};
    for (unsigned int i = 0; i < rhs_full.size(); i++){
        rhs_full[i] = rhs_double[i] + mass_factor[dim - 2] * rhs_mass[i];
    }
    TOC("RHS Eval")

    // The LHS matrix for a Dirichlet Laplace problem.
    TIC2
    Problem<dim> p_single = {mesh, mesh, laplace_single<dim>, dudn};
    auto matrix = interact_matrix(p_single, qs);
    TOC("Matrix construct on " + std::to_string(mesh.facets.size()) + " facets");


    // Apply the constraints to the RHS vector to get a condensed vector.
    auto rhs = constraints.get_reduced(rhs_full);
    
    // Actually solve the linear system by providing a function to evaluate
    // matrix vector products.
    int count = 0;
    auto dudn_solved_subset = solve_system(rhs, linear_solve_tol,
        [&] (std::vector<double>& x, std::vector<double>& y) {
            std::cout << "iteration " << count << std::endl;
            count++;
            TIC
            // Expand the condensed vector to get all the DOFs. This is done
            // each iteration of the linear solve so that the matrix can be 
            // left in its uncondensed form.
            auto x_full = constraints.get_all(x, n_dofs);
            auto y_mult = bem_mat_mult(matrix, n_dofs, x_full); 

            // Reduce the result of the matrix vector product.
            auto y_temp = constraints.get_reduced(y_mult);
            std::copy(y_temp.begin(), y_temp.end(), y.begin());
            TOC("Matrix multiply on " + 
                std::to_string(mesh.facets.size()) +
                " faces");
        });
    // Get all the constrained DOFs from the reduced DOF vector.
    auto dudn_solved = constraints.get_all(dudn_solved_subset, n_dofs);

    // Output the error and the solution 
    std::cout << error_inf(dudn_solved, dudn) << std::endl;
    hdf_out("laplace" + std::to_string(dim) + "d.hdf5", mesh, dudn_solved); 

    for(int i = 0; i < test_interior_pts.size(); i++) {
        auto obs_pt = test_interior_pts[i]; 
        ObsPt<dim> obs = {0.001, obs_pt, zeros<Vec<double,dim>>()};
       
        double double_layer = eval_integral_equation(p_double, qs, obs);
        double single_layer = eval_integral_equation(p_single, qs, obs);
        double result = single_layer - double_layer;
        double exact = fnc(obs_pt);
        double error = std::fabs(exact - result);
        if (error > 1e-2) {
            std::cout << "Failed with point: " << obs_pt << std::endl;
            std::cout << result << " " << exact << std::endl;
        }
    }
    // double obs_len_scale = get_len_scale(circle, 0, obs_quad_pts);
    // for(int i = 0; i < 100; i++) {
    //
    //     auto obs_pt3d = random_pt_sphere({center[0], center[1], 0.0}, obs_radius);
    //     Vec2<double> obs_pt = {obs_pt3d[0], obs_pt3d[1]};

    //     auto obs_normal = normalized(center - obs_pt);
    //     ObsPt<2> obs = {obs_len_scale, obs_pt, obs_normal};
    //    
    //     double double_layer = eval_integral_equation(p_double, qs, obs);
    //     double single_layer = eval_integral_equation(p_single, qs, obs);
    //     double result = single_layer - double_layer;
    //     double exact = harmonic_u(obs_pt);
    //     double error = std::fabs((exact - result) / exact);
    //     if (error > 1e-2) {
    //         std::cout << "Failed with point: " << obs_pt << std::endl;
    //         std::cout << "Exact: " << exact << std::endl;
    //         std::cout << "Result: " << result << std::endl << std::endl;
    //     }
    // }
}

#endif