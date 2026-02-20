from EasyjetHub.output.ttree.branch_manager import BranchManager, SystOption
from EasyjetHub.output.ttree.truth_jets import get_TopHiggs_jet_truth_labels
from EasyjetHub.steering.sample_metadata import is_at_least


def get_large_R_jet_branches(
    flags, tree_flags,
    input_container,
    output_prefix,
    lr_jet_type
):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

    jet_output_flags = tree_flags.collection_options.large_R_jets

    large_R_jet_branches = BranchManager(
        input_container,
        output_prefix,
        systematics_option=_syst_option,
        systematics_suffix_separator=flags.Analysis.systematics_suffix_separator
    )

    if tree_flags.slim_variables_with_syst:
        large_R_jet_branches.syst_only_for = ["pt", "m"]

    if lr_jet_type == "Topo":
        large_R_jet_branches.required_flags.append(flags.Analysis.do_large_R_Topo_jets)
    if lr_jet_type == "UFO":
        large_R_jet_branches.required_flags.append(flags.Analysis.do_large_R_UFO_jets)

    large_R_jet_branches.add_four_mom_branches(do_mass=True)

    # Similar run_selection as small_R_jets to avoid ordering issues
    if jet_output_flags.run_selection:
        large_R_jet_branches.variables += [
            ("isAnalysisJet_%SYS%", "char")
        ]
        for index in range(flags.Analysis.Large_R_jet.amount_leadingjet):
            large_R_jet_branches.variables += [
                (f"isjet{index+1}_%SYS%", "char")
            ]
    else:
        if flags.Analysis.do_overlap_removal:
            large_R_jet_branches.variables += [
                ("passesOR_%SYS%", "char")
            ]

    if flags.Analysis.Large_R_jet.runMuonJetPtCorr:
        if jet_output_flags.no_bjet_calib_p4:
            large_R_jet_branches.variables += [
                ("n_muons_%SYS%", "int")
            ]

            large_R_jet_branches.variables += [
                (f"NoBJetCalibMomentum_{var}", "float")
                for var in ["pt", "eta", "phi", "m"]
            ]
            if tree_flags.slim_variables_with_syst:
                large_R_jet_branches.syst_only_for += [
                    "NoBJetCalibMomentum_pt",
                    "NoBJetCalibMomentum_m",
                ]

    if flags.Input.isMC and jet_output_flags.truth_labels:
        large_R_jet_branches.variables += [
            ("GhostBHadronsFinalCount", "int")
        ]
        large_R_jet_branches.variables += get_large_R_jet_truth_labels(flags)

        if lr_jet_type == "Topo":
            large_R_jet_branches.variables += [
                ("R10TruthLabel_R21Consolidated", "int")
            ]
        if lr_jet_type == "UFO":
            large_R_jet_branches.variables += [
                ("GhostCHadronsFinalCount", "int"),
                ("R10TruthLabel_R21Precision_2022v1", "int"),
                ("R10TruthLabel_R22v1", "int")
            ]

    if jet_output_flags.substructure_info:
        large_R_jet_branches.variables += get_substructure_branches(flags, lr_jet_type)

    if (
        lr_jet_type == "UFO"
        and flags.Analysis.Large_R_jet.wtag_type
        and flags.Analysis.Large_R_jet.wtag_wp
    ):
        large_R_jet_branches.variables += get_wtag_branches(flags)

    is_valid_ptag = is_at_least(flags, "p5834")
    is_valid_for_v02 = is_at_least(flags, "p6490")
    is_valid_for_bjr_v01 = is_at_least(flags, "p6697")
    is_valid_for_gn3x = is_at_least(flags, "p7017")
    if (
        lr_jet_type == "UFO"
        and is_valid_for_gn3x
        and jet_output_flags.WTransformer_details
    ):
        large_R_jet_branches.variables += [
            ("WTransformer_massdec_ConstScore", "float"),
            ("WTransformer_ConstScore", "float")
        ]

    if lr_jet_type == "UFO" and is_valid_ptag:
        if jet_output_flags.btag_details:
            large_R_jet_branches.variables += get_large_R_gn2_branches(
                is_valid_for_v02
            )
        if flags.Analysis.Large_R_jet.flat_mass_selections:
            large_R_jet_branches.variables += get_large_R_fm_tag_branches(flags)

    if (
        lr_jet_type == "UFO"
        and is_valid_for_bjr_v01
        and jet_output_flags.bjet_regression
    ):
        # p6697 introduced some bjr variables, but by p7017 one was switched our for
        # the ext version. Here we check if we're between those two p-tags to decide
        # which to use.
        prefix = "bJR10v00Ext" if is_valid_for_gn3x else "bJR10v00"
        large_R_jet_branches.variables += [
            (prefix + "_pt", "float"),
            (prefix + "_mass", "float"),
            ("bJR10v01_pt", "float"),
            ("bJR10v01_mass", "float")
        ]

    if (
        lr_jet_type == "UFO"
        and is_valid_for_gn3x
        and (
            jet_output_flags.btag_details or jet_output_flags.tautag_details
        )
    ):
        taggers = flags.Analysis.Large_R_jet.variables_in_ptag.p7017
        if jet_output_flags.tautag_details:
            large_R_jet_branches.variables += [
                ('GN2XTauV00_htt_score', "float"), ('GN3XPV01_htt_score', "float")
            ]
        large_R_jet_branches.variables += [
            (tag, "float") for tag in taggers
        ]

    large_R_jet_branches.variables += jet_output_flags.extra_variables
    if flags.Input.isMC:
        large_R_jet_branches.variables += jet_output_flags.mc_extra_variables

    return large_R_jet_branches.get_output_list()


def get_substructure_branches(flags, lr_jet_type):
    substructure_vars = [
        (f"Tau{i}_wta", "float") for i in range(1, 4)
    ]
    substructure_vars += [
        (f"ECF{i}", "float") for i in range(1, 4)
    ]
    substructure_vars += [
        ("Split12", "float"),
        ("Split23", "float")
    ]
    # Trimmed jet vars
    if lr_jet_type == "Topo":
        substructure_vars += [
            ("NTrimSubjets", "int"),
            ("TrackSumPt", "float")
        ]

    substructure_vars += [
        ("D2", "float")
    ]
    return substructure_vars


def get_large_R_gn2_branches(is_valid_for_v02):
    gn2_branches = [
        (f"GN2Xv01_p{c}", "float")
        for c in ["hbb", "hcc", "top", "qcd"]
    ]
    if is_valid_for_v02:
        gn2_branches += [
            (f"GN2Xv02_p{c}", "float")
            for c in ["hbb", "hcc", "top", "qcd"]
        ]
    return gn2_branches


def get_large_R_gn2_tag_branches(flags):
    gn2_tag_branches = []
    for wp in flags.Analysis.Large_R_jet.GN2X_hbb_wps:
        gn2_tag_branches += [
            ("xbb_select_GN2Xv01_" + wp, "int")
        ]
        # Disable SF for now to avoid warnings with preliminary json
        # if flags.Input.isMC:
        #    gn2_tag_branches += ["xbb_effSF_GN2Xv01_" + wp + "_NOSYS"]
    return gn2_tag_branches


def get_large_R_fm_tag_branches(flags):
    fm_tag_branches = []
    for tagger, tagger_cfg in flags.Analysis.Large_R_jet.flat_mass_selections.items():
        for wp_name in tagger_cfg['wps']:
            fm_tag_branches += [(f"xbb_select_{tagger}_{wp_name}", "int")]
    return fm_tag_branches


def get_large_R_jet_truth_labels(flags):
    parent_bosons = ["Higgs", "Scalar", "Top"]

    truth_labels = []
    if not flags.Input.isPHYSLITE and flags.Analysis.Large_R_jet.do_parent_decoration:
        truth_labels += [
            (f"parent{p}NMatchedChildren", "unsigned_char")
            for p in parent_bosons
        ]

    truth_labels += get_TopHiggs_jet_truth_labels(flags)

    return truth_labels


def get_wtag_branches(flags):
    wtag_branches = []

    wtag_type = flags.Analysis.Large_R_jet.wtag_type
    wtag_wp = flags.Analysis.Large_R_jet.wtag_wp
    wtag_extra_wps = list(flags.Analysis.Large_R_jet.wtag_extra_wps or [])

    for wp in [wtag_wp] + wtag_extra_wps:
        wtag_branches += [
            (f"{wtag_type}{wp}Tagger_Tagged", "char"),
            (f"{wtag_type}{wp}Tagger_Score", "float")
        ]

    return wtag_branches
