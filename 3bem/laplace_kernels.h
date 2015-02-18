#ifndef __ddddddddddddEQwQWEA_LAPLACE_KERNELS_H
#define __ddddddddddddEQwQWEA_LAPLACE_KERNELS_H

#include "vec_ops.h"
#include "kernel.h"

namespace tbem {

/* Laplace/Poisson equation kernels. */
template <size_t dim>
struct LaplaceSingle;
template <size_t dim>
struct LaplaceDouble;
template <size_t dim>
struct LaplaceHypersingular;

template <>
struct LaplaceSingle<3>: ScalarKernel<3> 
{
    Vec1<Vec1<double>> operator()(double r2, const Vec<double,3>& delta,
                      const Vec<double,3>& nsrc, const Vec<double,3>& nobs) const {
        return {{{1.0 / (4.0 * M_PI * std::sqrt(r2))}}};
    }
};

template <>
struct LaplaceDouble<3>: ScalarKernel<3> 
{
    Vec1<Vec1<double>> operator()(double r2, const Vec<double,3>& delta,
                      const Vec<double,3>& nsrc, const Vec<double,3>& nobs) const {
        return {{{dot_product(nsrc, delta) / (4 * M_PI * r2 * std::sqrt(r2))}}};
    }
};

template <>
struct LaplaceSingle<2>: ScalarKernel<2>
{
    Vec1<Vec1<double>> operator()(double r2, const Vec<double,2>& delta,
                      const Vec<double,2>& nsrc, const Vec<double,2>& nobs) const {
        return {{{std::log(std::sqrt(r2)) / (2 * M_PI)}}};
    }
};

template <>
struct LaplaceDouble<2>: ScalarKernel<2>
{
    Vec1<Vec1<double>> operator()(double r2, const Vec<double,2>& delta,
                      const Vec<double,2>& nsrc, const Vec<double,2>& nobs) const {
        return {{{dot_product(nsrc, delta) / (2 * M_PI * r2)}}};
    }
};

template <>
struct LaplaceHypersingular<2>: ScalarKernel<2>
{
    Vec1<Vec1<double>> operator()(double r2, const Vec<double,2>& delta,
                      const Vec<double,2>& nsrc, const Vec<double,2>& nobs) const {
        return {{{((-dot_product(nobs, nsrc) / r2) + 
                ((2 * dot_product(nsrc, delta) * dot_product(nobs, delta)) / (r2 * r2)))
            / (2 * M_PI)}}};
    }
};

} // END namespace tbem

#endif
