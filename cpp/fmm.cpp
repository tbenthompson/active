#include <omp.h>
#include "fmm.h"
#include "octree.h"
#include "numerics.h"
#include "direct.h"
#include <cassert>
#include "util.h"
#include <list>

namespace tbem {

std::array<std::vector<double>,3> get_3d_expansion_nodes(int n_exp_pts) {
    auto nodes = cheb_pts_first_kind(n_exp_pts);
    std::array<std::vector<double>,3> nodes_3d;
    for(int d0 = 0; d0 < n_exp_pts; d0++) {
        for(int d1 = 0; d1 < n_exp_pts; d1++) {
            for(int d2 = 0; d2 < n_exp_pts; d2++) {
                nodes_3d[0].push_back(nodes[d0]);
                nodes_3d[1].push_back(nodes[d1]);
                nodes_3d[2].push_back(nodes[d2]);
            }
        }
    }
    return nodes_3d;
}

double interp_operator(const Box& bounds,
                       double nodex, double nodey, double nodez,
                       double ptx, double pty, double ptz,
                       int n_exp_pts) {
    double effect = 1.0;
    // X interp
    double x_hat = real_to_ref(ptx, bounds.min_corner[0], bounds.max_corner[0]);
    effect *= s_n_fast(nodex, x_hat, n_exp_pts);
    // Y interp
    x_hat = real_to_ref(pty, bounds.min_corner[1], bounds.max_corner[1]);
    effect *= s_n_fast(nodey, x_hat, n_exp_pts);
    // Z interp
    x_hat = real_to_ref(ptz, bounds.min_corner[2], bounds.max_corner[2]);
    effect *= s_n_fast(nodez, x_hat, n_exp_pts);
    return effect;
}


FMMInfo::FMMInfo(Kernel kernel, const Octree& src, std::vector<double>& values, 
                 const Octree& obs, int n_exp_pts, double mac2):
    mac2(mac2),
    n_exp_pts(n_exp_pts),
    nodes(get_3d_expansion_nodes(n_exp_pts)),
    kernel(kernel),
    src_oct(src),
    multipole_weights(src_oct.cells.size() * nodes[0].size(), 0.0),
    values(values),
    obs_oct(obs),
    local_weights(obs_oct.cells.size() * nodes[0].size(), 0.0),
    obs_effect(obs_oct.elements[0].size(), 0.0),
    p2p_jobs(obs_oct.cells.size()),
    m2l_jobs(obs_oct.cells.size()),
    m2p_jobs(obs_oct.cells.size())
{}

void FMMInfo::P2M_pts_cell(int m_cell_idx) {
    const auto cell = src_oct.cells[m_cell_idx];
    int cell_start_idx = m_cell_idx * nodes[0].size();
    for(unsigned int i = cell.begin; i < cell.end; i++) {
        for(unsigned int j = 0; j < nodes[0].size(); j++) {
            int node_idx = cell_start_idx + j;   
            double P2M_kernel = interp_operator(cell.bounds, nodes[0][j],
                                                nodes[1][j], nodes[2][j],
                                                src_oct.elements[0][i], 
                                                src_oct.elements[1][i],
                                                src_oct.elements[2][i],
                                                n_exp_pts);
            multipole_weights[node_idx] += values[i] * P2M_kernel;
        }
    }
}

void FMMInfo::P2M_helper(int m_cell_idx) {
    const auto cell = src_oct.cells[m_cell_idx];
    // If no children, do leaf P2M
    if (cell.is_leaf) {
        P2M_pts_cell(m_cell_idx);
        return;
    }
#pragma omp parallel if(cell.level < 1)
    {
#pragma omp single
        for (int c = 0; c < 8; c++) {
            int child_idx = cell.children[c];
            if (child_idx == -1) {
                continue;
            }
            // bottom-up tree traversal, recurse before doing work.
#pragma omp task
            P2M_helper(child_idx);
        }
#pragma omp taskwait
#pragma omp single
        for (int c = 0; c < 8; c++) {
            int child_idx = cell.children[c];
            if (child_idx == -1) {
                continue;
            }
            auto child = src_oct.cells[child_idx];
            for(unsigned int i = 0; i < nodes[0].size(); i++) {
                int child_node_idx = nodes[0].size() * child_idx + i;
                std::array<double,3> mapped_pt_src;
                for(int d = 0; d < 3; d++) {
                    mapped_pt_src[d] = ref_to_real(nodes[d][i],
                                                   child.bounds.min_corner[d],
                                                   child.bounds.max_corner[d]);
                }
                for(unsigned int j = 0; j < nodes[0].size(); j++) {
                    int node_idx = nodes[0].size() * m_cell_idx + j;   
                    double P2M_kernel = interp_operator(cell.bounds,
                                                        nodes[0][j],
                                                        nodes[1][j],
                                                        nodes[2][j],
                                                        mapped_pt_src[0],
                                                        mapped_pt_src[1],
                                                        mapped_pt_src[2],
                                                        n_exp_pts);
                    multipole_weights[node_idx] += 
                        multipole_weights[child_node_idx] * P2M_kernel;
                }
            }
        }
    }
}

void FMMInfo::P2M() {
    P2M_helper(src_oct.get_root_index()); 
}

void FMMInfo::P2P_cell_pt(const OctreeCell& m_cell, int pt_idx) {
    for(unsigned int i = m_cell.begin; i < m_cell.end; i++) {
        const double P2P_kernel = kernel(obs_oct.elements[0][pt_idx],
                                         obs_oct.elements[1][pt_idx],
                                         obs_oct.elements[2][pt_idx],
                                         src_oct.elements[0][i],
                                         src_oct.elements[1][i],
                                         src_oct.elements[2][i]
                                         );
        obs_effect[pt_idx] += values[i] * P2P_kernel;
    }
}

void FMMInfo::P2P_cell_cell(int m_cell_idx, int l_cell_idx) {
    auto m_cell = src_oct.cells[m_cell_idx];
    auto l_cell = obs_oct.cells[l_cell_idx];
    for(unsigned int i = l_cell.begin; i < l_cell.end; i++) {
        P2P_cell_pt(m_cell, i);
    }
}

void FMMInfo::M2P_cell_pt(const Box& m_cell_bounds,
                 int m_cell_idx, int pt_idx) {
    int cell_start_idx = m_cell_idx * nodes[0].size();
    for(unsigned int j = 0; j < nodes[0].size(); j++) {
        int node_idx = cell_start_idx + j;   
        std::array<double,3> src_node_loc;
        for(int d = 0; d < 3; d++) {
            src_node_loc[d] = ref_to_real(nodes[d][j],
                                          m_cell_bounds.min_corner[d],
                                          m_cell_bounds.max_corner[d]);
        }
        const double M2P_kernel = kernel(obs_oct.elements[0][pt_idx],
                                         obs_oct.elements[1][pt_idx],
                                         obs_oct.elements[2][pt_idx],
                                         src_node_loc[0],
                                         src_node_loc[1],
                                         src_node_loc[2]
                                         );
        obs_effect[pt_idx] += multipole_weights[node_idx] * M2P_kernel;
    }
}

void FMMInfo::M2P_cell_cell(int m_cell_idx, int l_cell_idx) {
    auto m_cell_bounds = src_oct.cells[m_cell_idx].bounds;
    auto l_cell = obs_oct.cells[l_cell_idx];
    for (int i = l_cell.begin; i < (int)l_cell.end; i++) {
        M2P_cell_pt(m_cell_bounds, m_cell_idx, i);
    }
}

void FMMInfo::treecode_process_cell(const OctreeCell& cell, int cell_idx, int pt_idx) {
    const double dist_squared = dist2({{obs_oct.elements[0][pt_idx],
                                       obs_oct.elements[1][pt_idx],
                                       obs_oct.elements[2][pt_idx]}},
                                       cell.bounds.center);
    const double radius_squared = hypot2(cell.bounds.half_width); 
    if (dist_squared > mac2 * radius_squared) {
        M2P_cell_pt(cell.bounds, cell_idx, pt_idx);    
        return;
    } else if (cell.is_leaf) {
        P2P_cell_pt(cell, pt_idx);
        return;
    }
    treecode_helper(cell, pt_idx);
}

void FMMInfo::treecode_helper(const OctreeCell& cell, int pt_idx) {
    for (int c = 0; c < 8; c++) {
        const int child_idx = cell.children[c];
        if (child_idx == -1) {
            continue;
        }
        const auto child = src_oct.cells[child_idx];
        treecode_process_cell(child, child_idx, pt_idx);
    }
}

void FMMInfo::treecode() {
    const int root_idx = src_oct.get_root_index();
    const auto root = src_oct.cells[root_idx];
#pragma omp parallel for
    for(unsigned int i = 0; i < obs_oct.n_elements(); i++) {
        treecode_process_cell(root, root_idx, i);
    }
}

void FMMInfo::M2L_cell_cell(int m_cell_idx, int l_cell_idx) {
    int m_cell_start_idx = m_cell_idx * nodes[0].size();
    int l_cell_start_idx = l_cell_idx * nodes[0].size();
    auto m_cell_bounds = src_oct.cells[m_cell_idx].bounds;
    auto l_cell_bounds = obs_oct.cells[l_cell_idx].bounds;
    for(unsigned int i = 0; i < nodes[0].size(); i++) {
        int l_node_idx = l_cell_start_idx + i;
        std::array<double,3> obs_node_loc;
        for(int d = 0; d < 3; d++) {
            obs_node_loc[d] = ref_to_real(nodes[d][i],
                                          l_cell_bounds.min_corner[d],
                                          l_cell_bounds.max_corner[d]);
        }

        for(unsigned int j = 0; j < nodes[0].size(); j++) {
            int m_node_idx = m_cell_start_idx + j;   
            std::array<double,3> src_node_loc;
            for(int d = 0; d < 3; d++) {
                src_node_loc[d] = ref_to_real(nodes[d][j],
                                              m_cell_bounds.min_corner[d],
                                              m_cell_bounds.max_corner[d]);
            }

            double M2L_kernel = kernel(obs_node_loc[0], obs_node_loc[1],
                                       obs_node_loc[2], src_node_loc[0],
                                       src_node_loc[1], src_node_loc[2]);
            local_weights[l_node_idx] += 
                multipole_weights[m_node_idx] * M2L_kernel;
        }
    }
}

void FMMInfo::L2P_cell_pts(int l_cell_idx) {
    const auto cell = obs_oct.cells[l_cell_idx];
    int cell_start_idx = l_cell_idx * nodes[0].size();
    for(unsigned int i = cell.begin; i < cell.end; i++) {
        for(unsigned int j = 0; j < nodes[0].size(); j++) {
            int node_idx = cell_start_idx + j;   
            double L2P_kernel = interp_operator(cell.bounds,
                                                nodes[0][j],
                                                nodes[1][j],
                                                nodes[2][j],
                                                obs_oct.elements[0][i],
                                                obs_oct.elements[1][i],
                                                obs_oct.elements[2][i],
                                                n_exp_pts);
            obs_effect[i] += local_weights[node_idx] * L2P_kernel;
        }
    }
}

void FMMInfo::L2P_helper(int l_cell_idx) {
    const auto cell = obs_oct.cells[l_cell_idx];
    // If no children, do leaf P2M
    if (cell.is_leaf) {
        L2P_cell_pts(l_cell_idx);
        return;
    }

#pragma omp parallel if(cell.level < 1)
    {
#pragma omp single
        for (int c = 0; c < 8; c++) {
            int child_idx = cell.children[c];
            if (child_idx == -1) {
                continue;
            }

            // top-down tree traversal, recurse after doing work
            auto child = obs_oct.cells[child_idx];
            for(unsigned int i = 0; i < nodes[0].size(); i++) {
                int child_node_idx = nodes[0].size() * child_idx + i;
                std::array<double,3> mapped_local_pt;
                for(int d = 0; d < 3; d++) {
                    mapped_local_pt[d] = ref_to_real(nodes[d][i],
                                                     child.bounds.min_corner[d],
                                                     child.bounds.max_corner[d]);
                }
                for(unsigned int j = 0; j < nodes[0].size(); j++) {
                    int node_idx = nodes[0].size() * l_cell_idx + j;   
                    double L2P_kernel = interp_operator(cell.bounds,
                                                        nodes[0][j],
                                                        nodes[1][j],
                                                        nodes[2][j],
                                                        mapped_local_pt[0],
                                                        mapped_local_pt[1],
                                                        mapped_local_pt[2],
                                                        n_exp_pts);
                    local_weights[child_node_idx] += 
                        local_weights[node_idx] * L2P_kernel;
                }
            }

#pragma omp task 
            L2P_helper(child_idx);
        }
    }
}

void FMMInfo::L2P() {
    L2P_helper(obs_oct.get_root_index()); 
}

void FMMInfo::fmm_process_cell_pair(const OctreeCell& m_cell, int m_cell_idx,
                                    const OctreeCell& l_cell, int l_cell_idx) {
    const double dist_squared = dist2(l_cell.bounds.center, m_cell.bounds.center);

    //Logic is:
    // If the cell is far enough away, as determined by comparing 
    // the ratio of distance to radius with the multipole acceptance criteria (MAC)
    // then perform an interaction. Which interaction depends on how costly each 
    // interaction is. The number of particles in each cell are compared. If the
    // number of particles in the source cell is greater than the
    // number of expansion pts, then the M2L or M2P is used depending on the
    // number of particles in the observation cell. If the number of particles
    // is low enough not to justify the M2L or M2P operators, the P2P is used.
    //
    // Otherwise, if both cells are leaves, perform a P2P.
    //
    // If neither case is true, recurse to comparing the child cells.
    if (2 * dist_squared > mac2 * (m_cell.bounds.radius2 + l_cell.bounds.radius2)) {
        if (l_cell.end - l_cell.begin < nodes[0].size()) {
            if (m_cell.end - m_cell.begin < nodes[0].size()) {
                p2p_jobs[l_cell_idx].push_back(m_cell_idx);
            } else {
                m2p_jobs[l_cell_idx].push_back(m_cell_idx);
            }
        } else {
            m2l_jobs[l_cell_idx].push_back(m_cell_idx);
        }
    } else if (m_cell.is_leaf && l_cell.is_leaf) {
        p2p_jobs[l_cell_idx].push_back(m_cell_idx);
    } else {
        fmm_process_children(m_cell, m_cell_idx, l_cell, l_cell_idx);
    }
}

void FMMInfo::fmm_process_children(const OctreeCell& m_cell, int m_cell_idx,
                                   const OctreeCell& l_cell, int l_cell_idx) {
    //preferentially refine l_cell.
    if ((l_cell.level <= m_cell.level && !l_cell.is_leaf) || m_cell.is_leaf) {
        //refine l_cell
        assert(!l_cell.is_leaf);

// Threads must be split along observation cell lines. Otherwise multiple
// threads will be working on the same output and locking and synchronization
// will be required. So, the task farming is only done when an l_cell is refined.
#pragma omp parallel if(l_cell.level < 1)
        {
#pragma omp single
        for (int c = 0; c < 8; c++) {
            const int l_child_idx = l_cell.children[c];
            if (l_child_idx != -1) {
#pragma omp task
                fmm_process_cell_pair(m_cell, m_cell_idx, 
                                      obs_oct.cells[l_child_idx], l_child_idx);
            }
        }
        }
    } else {
        //refine m_cell
        assert(!m_cell.is_leaf);

        for (int c = 0; c < 8; c++) {
            const int m_child_idx = m_cell.children[c];
            if (m_child_idx != -1) {
                fmm_process_cell_pair(src_oct.cells[m_child_idx], m_child_idx,
                                  l_cell, l_cell_idx);
            }
        }
    }
}

template <void (FMMInfo::*op) (int, int)>
void exec_jobs(FMMInfo& fmm_info, std::vector<std::vector<int>>& job_set, std::string name) {
    long interactions = 0;
#pragma omp parallel for
    for (unsigned int l_idx = 0; l_idx < job_set.size(); l_idx++) {
        for (unsigned int j = 0; j < job_set[l_idx].size(); j++) {
            int m_idx = job_set[l_idx][j];
            (fmm_info.*op)(m_idx, l_idx);

            auto m_cell = fmm_info.src_oct.cells[m_idx];
            auto l_cell = fmm_info.obs_oct.cells[l_idx];
            if (name == "M2L") {
                interactions += pow(fmm_info.n_exp_pts, 6);
            } else if (name == "M2P") {
                interactions += pow(fmm_info.n_exp_pts, 3) * (l_cell.end - l_cell.begin);
            } else {
                interactions += (m_cell.end - m_cell.begin) * (l_cell.end - l_cell.begin);
            }
        }
    }
    // std::cout << "Billions of interactions for " << name
    //           << ": " << interactions/1e9 << " and an average of: "
    //           << (interactions / job_set.size()) << std::endl;
}

void FMMInfo::fmm() {
    // TIC;
    const int src_root_idx = src_oct.get_root_index();
    const auto src_root = src_oct.cells[src_root_idx];
    const int obs_root_idx = obs_oct.get_root_index();
    const auto obs_root = obs_oct.cells[obs_root_idx];
    fmm_process_cell_pair(src_root, src_root_idx, obs_root, obs_root_idx);
    // TOC("Tree traverse");

    // TIC2
    exec_jobs<&FMMInfo::P2P_cell_cell>(*this, p2p_jobs, "P2P");
    // TOC("P2P");

    // TIC2
    exec_jobs<&FMMInfo::M2L_cell_cell>(*this, m2l_jobs, "M2L");
    // TOC("M2L");

    // TIC2
    exec_jobs<&FMMInfo::M2P_cell_cell>(*this, m2p_jobs, "M2P");
    // TOC("M2P");
}

} // END namespace tbem