def get_unit_test_info(c):
    unit_test_info = dict()
    unit_test_info['basis'] = dict()
    unit_test_info['basis']['src'] = 'test_basis'
    unit_test_info['basis']['lib_srcs'] = []
    unit_test_info['basis']['link_lib'] = True
    unit_test_info['block_dof_map'] = dict()
    unit_test_info['block_dof_map']['src'] = 'test_block_dof_map'
    unit_test_info['block_dof_map']['lib_srcs'] = []
    unit_test_info['block_dof_map']['link_lib'] = True
    unit_test_info['closest_pt'] = dict()
    unit_test_info['closest_pt']['src'] = 'test_closest_pt'
    unit_test_info['closest_pt']['lib_srcs'] = []
    unit_test_info['closest_pt']['link_lib'] = True
    unit_test_info['constraint'] = dict()
    unit_test_info['constraint']['src'] = 'test_constraint'
    unit_test_info['constraint']['lib_srcs'] = []
    unit_test_info['constraint']['link_lib'] = True
    unit_test_info['constraint_matrix'] = dict()
    unit_test_info['constraint_matrix']['src'] = 'test_constraint_matrix'
    unit_test_info['constraint_matrix']['lib_srcs'] = []
    unit_test_info['constraint_matrix']['link_lib'] = True
    unit_test_info['continuity_builder'] = dict()
    unit_test_info['continuity_builder']['src'] = 'test_continuity_builder'
    unit_test_info['continuity_builder']['lib_srcs'] = []
    unit_test_info['continuity_builder']['link_lib'] = True
    unit_test_info['dense_builder'] = dict()
    unit_test_info['dense_builder']['src'] = 'test_dense_builder'
    unit_test_info['dense_builder']['lib_srcs'] = []
    unit_test_info['dense_builder']['link_lib'] = True
    unit_test_info['elastic_kernels'] = dict()
    unit_test_info['elastic_kernels']['src'] = 'test_elastic_kernels'
    unit_test_info['elastic_kernels']['lib_srcs'] = []
    unit_test_info['elastic_kernels']['link_lib'] = True
    unit_test_info['fmm'] = dict()
    unit_test_info['fmm']['src'] = 'test_fmm'
    unit_test_info['fmm']['lib_srcs'] = []
    unit_test_info['fmm']['link_lib'] = True
    unit_test_info['function'] = dict()
    unit_test_info['function']['src'] = 'test_function'
    unit_test_info['function']['lib_srcs'] = []
    unit_test_info['function']['link_lib'] = True
    unit_test_info['integral_term'] = dict()
    unit_test_info['integral_term']['src'] = 'test_integral_term'
    unit_test_info['integral_term']['lib_srcs'] = [c['subdirs']['src_dir'] + '/integral_term']
    unit_test_info['integral_term']['link_lib'] = True
    unit_test_info['mass_operator'] = dict()
    unit_test_info['mass_operator']['src'] = 'test_mass_operator'
    unit_test_info['mass_operator']['lib_srcs'] = []
    unit_test_info['mass_operator']['link_lib'] = True
    unit_test_info['mesh'] = dict()
    unit_test_info['mesh']['src'] = 'test_mesh'
    unit_test_info['mesh']['lib_srcs'] = []
    unit_test_info['mesh']['link_lib'] = True
    unit_test_info['numbers'] = dict()
    unit_test_info['numbers']['src'] = 'test_numbers'
    unit_test_info['numbers']['lib_srcs'] = []
    unit_test_info['numbers']['link_lib'] = True
    unit_test_info['numerics'] = dict()
    unit_test_info['numerics']['src'] = 'test_numerics'
    unit_test_info['numerics']['lib_srcs'] = []
    unit_test_info['numerics']['link_lib'] = True
    unit_test_info['octree'] = dict()
    unit_test_info['octree']['src'] = 'test_octree'
    unit_test_info['octree']['lib_srcs'] = []
    unit_test_info['octree']['link_lib'] = True
    unit_test_info['operator'] = dict()
    unit_test_info['operator']['src'] = 'test_operator'
    unit_test_info['operator']['lib_srcs'] = []
    unit_test_info['operator']['link_lib'] = True
    unit_test_info['quadrature'] = dict()
    unit_test_info['quadrature']['src'] = 'test_quadrature'
    unit_test_info['quadrature']['lib_srcs'] = [c['subdirs']['src_dir'] + '/quadrature']
    unit_test_info['quadrature']['link_lib'] = True
    unit_test_info['richardson'] = dict()
    unit_test_info['richardson']['src'] = 'test_richardson'
    unit_test_info['richardson']['lib_srcs'] = []
    unit_test_info['richardson']['link_lib'] = False
    unit_test_info['vec'] = dict()
    unit_test_info['vec']['src'] = 'test_vec'
    unit_test_info['vec']['lib_srcs'] = []
    unit_test_info['vec']['link_lib'] = True
    unit_test_info['vertex_iterator'] = dict()
    unit_test_info['vertex_iterator']['src'] = 'test_vertex_iterator'
    unit_test_info['vertex_iterator']['lib_srcs'] = []
    unit_test_info['vertex_iterator']['link_lib'] = True
    return unit_test_info

