def get_selected_objects_branches(flags, analysis):
    object_level_branches, *_ = get_selected_objects_branches_variables(
        flags, analysis)
    return object_level_branches


def get_selected_objects_branches_variables(flags, analysis):
    branches = []
    float_variable_names = []
    int_variable_names = []

    obj_list = ["Jet", "Jet_b", "Jet_c", "Jet_l", "LargeRJet",
                "Photon", "Electron", "Muon", "Tau", "Lepton"]
    for obj_type in obj_list:
        _branches, _float_variable_names, _int_variable_names = \
            get_selected_obj_branches_variables(flags, analysis, obj_type)
        branches += _branches
        float_variable_names += _float_variable_names
        int_variable_names += _int_variable_names

    return branches, float_variable_names, int_variable_names


def get_selected_obj_branches_variables(flags, analysis, obj_type):
    branches = []
    float_variable_names = []
    int_variable_names = []

    sys_suffix = "%SYS%" if flags.Analysis.do_CP_systematics else "NOSYS"

    jet_config_map = {
        'Jet': {
            'var_list': flags.Analysis.Small_R_jet.variables_allJets,
            'int_var_list': flags.Analysis.Small_R_jet.variables_int_allJets,
            'amount': flags.Analysis.Small_R_jet.amount,
        },
        'Jet_b': {
            'var_list': flags.Analysis.Small_R_jet.variables_bjets,
            'int_var_list': flags.Analysis.Small_R_jet.variables_int_bjets,
            'amount': flags.Analysis.Small_R_jet.amount_bjet,
        },
        'Jet_c': {
            'var_list': flags.Analysis.Small_R_jet.variables_cjets,
            'int_var_list': flags.Analysis.Small_R_jet.variables_int_cjets,
            'amount': flags.Analysis.Small_R_jet.amount_cjet,
        },
        'Jet_l': {
            'var_list': flags.Analysis.Small_R_jet.variables_ljets,
            'int_var_list': flags.Analysis.Small_R_jet.variables_int_ljets,
            'amount': flags.Analysis.Small_R_jet.amount_ljet,
        },
        'LargeRJet': {
            'var_list': flags.Analysis.Large_R_jet.variables,
            'int_var_list': flags.Analysis.Large_R_jet.variables_int,
            'amount': flags.Analysis.Large_R_jet.amount,
        }
    }

    if "Jet" in obj_type:
        config = jet_config_map[obj_type]
    else:
        config = {
            'var_list': flags.Analysis[obj_type]["variables"],
            'int_var_list': flags.Analysis[obj_type]["variables_int"],
            'amount': flags.Analysis[obj_type]["amount"],
        }

    for var in config['int_var_list'] + config['var_list']:
        if not flags.Input.isMC and "SF" in var:
            continue
        for index in range(config['amount']):
            index_str = "" if config['amount'] == 1 else f"{index+1}"
            if var in config['var_list']:
                float_variable_names += [f"{obj_type}{index_str}_{var}"]
            if var in config['int_var_list']:
                int_variable_names += [f"{obj_type}{index_str}_{var}"]

            branch_str = (
                f"EventInfo.{obj_type}{index_str}_{var}_{sys_suffix} "
                f"-> {analysis}_{obj_type}{index_str}_{var}"
                f"{flags.Analysis.systematics_suffix_separator}{sys_suffix}"
            )
            branches += [branch_str]

    return branches, float_variable_names, int_variable_names
