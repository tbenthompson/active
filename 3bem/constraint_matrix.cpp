#include "constraint_matrix.h"

namespace tbem {

bool is_constrained(const ConstraintMatrix& dof_constraint_map, size_t dof) {
    const auto& it = dof_constraint_map.find(dof);
    if (it == dof_constraint_map.end()) {
        return false;
    }
    return true;
}

RearrangedConstraintEQ make_lower_triangular(const ConstraintEQ& c,
                                             const ConstraintMatrix& matrix) 
{
    if (c.terms.size() == 0) {
        std::string msg = "Function: make_lower_triangular has found either an empty constraint or a cyclic set of constraints.";
        throw std::invalid_argument(msg);
    }

    auto last_dof_index = find_last_dof_index(c);
    auto last_dof = c.terms[last_dof_index].dof;
    if (is_constrained(matrix, last_dof)) {
        const auto& last_dof_constraint = matrix.find(last_dof)->second;
        ConstraintEQ c_subs = substitute(c, last_dof_index, last_dof_constraint);
        ConstraintEQ c_subs_filtered = filter_zero_terms(c_subs);
        return make_lower_triangular(c_subs_filtered, matrix);
    }
    return isolate_term_on_lhs(c, last_dof_index);
}


ConstraintMatrix from_constraints(const std::vector<ConstraintEQ>& constraints) 
{
    ConstraintMatrix new_mat;

    for (size_t i = 0; i < constraints.size(); i++) {
        const auto& c = constraints[i];
        try {
            auto lower_tri_constraint = make_lower_triangular(c, new_mat);
            new_mat.insert(std::make_pair(lower_tri_constraint.constrained_dof,
                                          lower_tri_constraint));
        } catch (const std::invalid_argument& e) {
            continue;
        }
    }

    return ConstraintMatrix{new_mat};
};

std::vector<double>
distribute_vector(const ConstraintMatrix& matrix, const std::vector<double>& in,
                  size_t total_dofs) 
{
    std::vector<double> out(total_dofs); 

    size_t next_reduced_dof = 0;

    for (size_t dof_index = 0; dof_index < total_dofs; dof_index++) {
        if(is_constrained(matrix, dof_index)) {
            continue;
        }
        out[dof_index] = in[next_reduced_dof];
        next_reduced_dof++;
    }

    for (size_t dof_index = 0; dof_index < total_dofs; dof_index++) {
        if(!is_constrained(matrix, dof_index)) {
            continue;
        }
        auto constraint = matrix.find(dof_index)->second;
        auto val = constraint.rhs;
        for (size_t j = 0; j < constraint.terms.size(); j++) {
            assert(constraint.terms[j].dof < dof_index);
            val += constraint.terms[j].weight * out[constraint.terms[j].dof];
        }
        out[dof_index] = val;
    }
    return out;
}

void add_term_with_constraints(const ConstraintMatrix& matrix,
    std::vector<double>& modifiable_vec, const LinearTerm& entry) 
{
    if (!is_constrained(matrix, entry.dof)) {
        modifiable_vec[entry.dof] += entry.weight;
        return;
    }

    const auto& constraint = matrix.find(entry.dof)->second;
    const auto& terms = constraint.terms;
    std::vector<LinearTerm> out_terms;
    for (const auto& t: terms) {
        double recurse_weight = t.weight * entry.weight;
        modifiable_vec[t.dof] += recurse_weight;
    }
}

std::vector<double> condense_vector(const ConstraintMatrix& matrix,
    const std::vector<double>& all) 
{
    std::vector<double> condensed_dofs(all.size(), 0.0);
    for (int dof_idx = all.size() - 1; dof_idx >= 0; dof_idx--) {
        double condensed_value = condensed_dofs[dof_idx];
        condensed_dofs[dof_idx] = 0.0;
        LinearTerm term_to_add{(size_t)dof_idx, all[dof_idx] + condensed_value};
        add_term_with_constraints(matrix, condensed_dofs, term_to_add);
    }

    std::vector<double> out;
    for (size_t dof_idx = 0; dof_idx < all.size(); dof_idx++) {
        if (is_constrained(matrix, dof_idx)) {
            continue;
        }
        out.push_back(condensed_dofs[dof_idx]);
    }

    return out;
}

void add_entry_with_constraints(const ConstraintMatrix& row_cm, 
    const ConstraintMatrix& col_cm, Operator& modifiable_matrix,
    const MatrixEntry& entry) 
{
    int constrained_axis;
    int other_axis;
    const ConstraintMatrix* cm;
    if (!is_constrained(row_cm, entry.loc[0])) {
        if (!is_constrained(col_cm, entry.loc[1])) {
            size_t entry_idx = entry.loc[0] * modifiable_matrix.n_cols +
                               entry.loc[1];
            modifiable_matrix.data[entry_idx] += entry.value;
            return;
        }
        cm = &col_cm;
        constrained_axis = 1;
        other_axis = 0;
    } else {
        cm = &row_cm;
        constrained_axis = 0;
        other_axis = 1;
    }

    auto constrained_dof = entry.loc[constrained_axis];
    const auto& constraint = cm->find(constrained_dof)->second;
    const auto& terms = constraint.terms;

    for (const auto& t: terms) {
        double recurse_weight = t.weight * entry.value;
        size_t loc[2];
        loc[constrained_axis] = t.dof;
        loc[other_axis] = entry.loc[other_axis];
        assert(loc[constrained_axis] < constrained_dof);

        size_t entry_idx = loc[0] * modifiable_matrix.n_cols + loc[1];
        modifiable_matrix.data[entry_idx] += recurse_weight;
    }
}

Operator remove_constrained(const ConstraintMatrix& row_cm,
    const ConstraintMatrix& col_cm, const Operator& matrix) 
{
    auto n_rows_out = matrix.n_rows - row_cm.size();
    auto n_cols_out = matrix.n_cols - col_cm.size();
    auto n_elements_out = n_rows_out * n_cols_out;
    std::vector<double> out(n_elements_out);

    size_t out_row_idx = 0;
    for (size_t in_row_idx = 0; in_row_idx < matrix.n_rows; in_row_idx++) {
        if (is_constrained(row_cm, in_row_idx)) {
            continue;
        }
        size_t out_col_idx = 0;
        for (size_t in_col_idx = 0; in_col_idx < matrix.n_cols; in_col_idx++) {
            if (is_constrained(col_cm, in_col_idx)) {
                continue;
            }
            auto in_entry_idx = in_row_idx * matrix.n_cols + in_col_idx;
            auto out_entry_idx = out_row_idx * n_cols_out + out_col_idx;
            out[out_entry_idx] = matrix.data[in_entry_idx];

            out_col_idx++;
        }
        out_row_idx++;
    }

    return {n_rows_out, n_cols_out, out};
}

Operator condense_matrix(const ConstraintMatrix& row_cm,
    const ConstraintMatrix& col_cm, const Operator& matrix)
{
    Operator condensed {
        matrix.n_rows, matrix.n_cols,
        std::vector<double>(matrix.n_rows * matrix.n_cols, 0.0)
    };

    for (int row_idx = matrix.n_rows - 1; row_idx >= 0; --row_idx) {
        for (int col_idx = matrix.n_cols - 1; col_idx >= 0; --col_idx) {
            auto entry_idx = row_idx * matrix.n_cols + col_idx;
            auto condensed_value = condensed.data[entry_idx];
            condensed.data[entry_idx] = 0.0;
            MatrixEntry entry_to_add{
                {(size_t)row_idx, (size_t)col_idx},
                condensed_value + matrix.data[entry_idx]
            };

            add_entry_with_constraints(row_cm, col_cm, condensed, entry_to_add);
        }
    }
    
    return remove_constrained(row_cm, col_cm, condensed);
}

BlockOperator condense_block_operator(const std::vector<ConstraintMatrix>& row_cms,
    const std::vector<ConstraintMatrix>& col_cms, const BlockOperator& op) {
    std::vector<Operator> out_ops;
    for (size_t d1 = 0; d1 < op.n_comp_rows; d1++) {
        for (size_t d2 = 0; d2 < op.n_comp_cols; d2++) {
            out_ops.push_back(
                condense_matrix(row_cms[d1], col_cms[d2],
                    op.ops[d1 * op.n_comp_cols + d2])
            );
        }
    }
    return {
        op.n_comp_rows,
        op.n_comp_cols,
        out_ops
    };
}

} // END namespace tbem