#include "UnitTest++.h"
#include "block_dof_map.h"
#include "function.h"
#include "autocheck/autocheck.hpp"
namespace ac = autocheck;

using namespace tbem;

TEST(BuildBlockDOFMap) {
    auto dof_map = build_block_dof_map({1,2,3,4});
    CHECK_ARRAY_EQUAL(dof_map.start_positions, (std::vector<size_t>{0,1,3,6}), 4);
    CHECK_EQUAL(dof_map.n_dofs, 10);
    CHECK_EQUAL(dof_map.n_components, 4);
}

TEST(FromVectorXs) {
    BlockVectorX input{{1,2}, {3,4,5}, {6}};
    auto dof_map = block_dof_map_from_functions(input);
    CHECK_EQUAL(dof_map.n_components, 3);
    CHECK_EQUAL(dof_map.n_dofs, 6);
    CHECK_ARRAY_EQUAL(dof_map.start_positions, (std::vector<size_t>{0,2,5}), 3);
}

TEST(Concatenate) {
    BlockVectorX input{{1,2}, {3,4,5}, {6}};
    auto dof_map = block_dof_map_from_functions(input);
    auto concat_fnc = concatenate(dof_map, input);
    CHECK_ARRAY_EQUAL(concat_fnc, (VectorX{1,2,3,4,5,6}), 6);
}

TEST(Expand) {
    VectorX input{1,2,3,4,5};
    auto dof_map = build_block_dof_map({1,3,1});
    auto expanded = expand(dof_map, input);
    CHECK_ARRAY_EQUAL(expanded[0], (VectorX{1}), 1);
    CHECK_ARRAY_EQUAL(expanded[1], (VectorX{2,3,4}), 3);
    CHECK_ARRAY_EQUAL(expanded[2], (VectorX{5}), 1);
}

bool expand_concat_identity(const std::vector<std::vector<double>>& A) {
    BlockVectorX f(A.size());
    for (size_t i = 0; i < A.size(); i++) {
        f[i] = VectorX(A[i]);
    }
    auto dof_map = block_dof_map_from_functions(f);
    auto block_fnc = concatenate(dof_map, f); 
    auto expanded = expand(dof_map, block_fnc);
    if (A.size() != expanded.size()) {
        return false;
    }
    for (size_t i = 0; i < A.size(); i++) {
        if (A[i].size() != expanded[i].size()) {
            return false;
        }
        for (size_t j = 0; j < A[i].size(); j++) {
            if (fabs(A[i][j] - expanded[i][j]) > 1e-12) {
                return false;
            }
        }
    }
    return true;
}

TEST(ExpandConcatProperty) {
    ac::check<std::vector<std::vector<double>>>(expand_concat_identity);
}

int main(int, char const *[])
{
    return UnitTest::RunAllTests();
}
