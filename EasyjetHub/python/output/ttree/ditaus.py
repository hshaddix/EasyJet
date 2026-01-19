from EasyjetHub.output.ttree.branch_manager import BranchManager, SystOption


def get_ditau_branches(flags, tree_flags, input_container, output_prefix):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

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

    if flags.Analysis.DiTau.score_branches:
        ditau_branches.variables += [("omni_score", None)]

    if flags.Analysis.DiTau.truth_branches:
        ditau_branches.variables += [
            ("IsTruthHadronic", None),
            ("IsTruthMatched", None),
        ]
        ditau_branches.variables += [
            ("TruthVis" + var, None)
            for var in ["Mass", "DeltaR"]
        ]
        ditau_branches.variables += [
            ("TruthVis" + i + var, None)
            for i in ["Lead", "Sublead"]
            for var in ["Pt", "Eta", "Phi", "M"]
        ]
        ditau_branches.variables += [
            ("Truth" + i + "PdgID", None)
            for i in ["Lead", "Sublead"]
        ]

    return ditau_branches.get_output_list()
