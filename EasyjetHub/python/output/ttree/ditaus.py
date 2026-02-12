from EasyjetHub.output.ttree.branch_manager import BranchManager, SystOption


def get_ditau_branches(flags, tree_flags, input_container, output_prefix):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

    ditau_output_flags = tree_flags.collection_options.ditaus

    ditau_branches = BranchManager(
        input_container,
        output_prefix,
        systematics_option=_syst_option,
        systematics_suffix_separator=flags.Analysis.systematics_suffix_separator
    )

    if tree_flags.slim_variables_with_syst:
        ditau_branches.syst_only_for = ["pt"]

    ditau_branches.add_four_mom_branches(do_mass=True)

    id_wps = [flags.Analysis.DiTau.ID]
    if 'extra_wps' in flags.Analysis.DiTau:
        id_wps += flags.Analysis.DiTau.extra_wps

    ditau_branches.variables += [
        (f"baselineSelection_{id_wp}_%SYS%", None)
        for id_wp in id_wps
    ]

    if ditau_output_flags.score_branches:
        ditau_branches.variables += [("omni_score", "float")]

    if flags.Input.isMC and ditau_output_flags.truth_branches:
        ditau_branches.variables += [
            ("IsTruthHadronic", "char"),
            ("IsTruthMatched", "char"),
        ]
        ditau_branches.variables += [
            ("TruthVis" + var, "float")
            for var in ["Mass", "DeltaR"]
        ]
        ditau_branches.variables += [
            ("TruthVis" + i + var, "float")
            for i in ["Lead", "Sublead"]
            for var in ["Pt", "Eta", "Phi", "M"]
        ]
        ditau_branches.variables += [
            ("Truth" + i + "PdgID", "int")
            for i in ["Lead", "Sublead"]
        ]

    if ditau_output_flags.subjet_branches:
        for sjet in ["lead", "sublead"]:
            ditau_branches.variables += [
                (f"{sjet}Subjet{float_var}", "float")
                for float_var in ["Pt", "Eta", "Phi", "E"]
            ]
            ditau_branches.variables += [
                (f"{sjet}Subjet{int_var}", "int")
                for int_var in ["NTracks", "Charge"]
            ]

    ditau_branches.variables += ditau_output_flags.extra_variables
    if flags.Input.isMC:
        ditau_branches.variables += ditau_output_flags.mc_extra_variables

    return ditau_branches.get_output_list()
