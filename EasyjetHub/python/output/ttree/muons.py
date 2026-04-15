from EasyjetHub.output.ttree.branch_manager import BranchManager, SystOption
from EasyjetHub.output.ttree.truth_muons import get_TopHiggs_muon_truth_labels


def get_muon_branches(flags, tree_flags, input_container, output_prefix):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

    mu_output_flags = tree_flags.collection_options.muons

    muon_branches = BranchManager(
        input_container,
        output_prefix,
        systematics_option=_syst_option,
        systematics_suffix_separator=flags.Analysis.systematics_suffix_separator
    )

    if tree_flags.slim_variables_with_syst:
        muon_branches.syst_only_for = ["pt"]

    muon_branches.add_four_mom_branches(do_mass=False)
    muon_branches.variables += [
        ("charge", None)
    ]

    if mu_output_flags.iso_variables:
        muon_branches.variables += [
            ("neflowisol20", None),
            ("neflowisol20_CloseByCorr", None),
            ("topoetcone20", None),
            ("topoetcone20_CloseByCorr", None)
        ]
        muon_branches.variables += [
            (f"ptvarcone30_Nonprompt_All_MaxWeightTTVA_{suff}", None)
            for suff in ["pt500", "pt500_CloseByCorr", "pt1000", "pt1000_CloseByCorr"]
        ]

    if flags.Analysis.do_overlap_removal:
        muon_branches.variables += [
            ("passesOR_%SYS%", None)
        ]

    muon_branches.variables += [
        ("d0_%SYS%", None),
        ("d0sig_%SYS%", None),
        ("z0sintheta_%SYS%", None),
        ("z0sinthetasig_%SYS%", None)
    ]

    if flags.Analysis.Muon.do_reco_decoration:
        muon_branches.variables += [
            ("author", None),
            ("muonType", None)
        ]

    if flags.Analysis.Muon.MergeLRT:
        muon_branches.variables += [("isLRT", None)]

    if flags.Analysis.Muon.do_track_decoration:
        muon_branches.variables += [
            (f"idtrack_{var}", "float")
            for var in ["pt", "eta", "phi", "d0", "z0", "chi2OverDoF",
                        "nIBL", "nPIX", "nPIX_shared", "nSCT", "nSCT_shared"]
        ]
        muon_branches.variables += [
            (f"cbtrack_{var}", "float")
            for var in ["d0", "z0", "chi2OverDoF"]
        ]
        muon_branches.variables += [
            ("numberOfPrecisionLayers", None),
            ("numberOfPrecisionHoleLayers", None)
        ]

        if (flags.Input.ProcessingTags == ["StreamDAOD_LLP1"]):
            muon_branches.variables += [
                (f"idtrack_{var}", "float")
                for var in ["nNextToIBL", "nPIX_split", "nTRT"]
            ]

    id_wps = [f'{flags.Analysis.Muon.ID}_{flags.Analysis.Muon.Iso}']
    consider_extra_wp = any(len(wp) == 4 for wp in flags.Analysis.Muon.extra_wps)

    if 'extra_wps' in flags.Analysis.Muon:
        for wp in flags.Analysis.Muon.extra_wps:
            if len(wp) == 4:
                id_wps.append(
                    (
                        wp[0] + "_" + wp[1] + "_"
                        + str(wp[2]) + "_" + str(wp[3])
                    ).replace('.', 'p')
                )
            elif consider_extra_wp:
                id_wps.append(
                    (
                        wp[0] + "_" + wp[1] + "_"
                        + str(flags.Analysis.Muon.maxD0Significance) + "_"
                        + str(flags.Analysis.Muon.maxDeltaZ0SinTheta)
                    ).replace('.', 'p')
                )
            else:
                id_wps.append(wp[0] + "_" + wp[1])

    muon_branches.variables += [
        (f"baselineSelection_{id_wp}_%SYS%", None)
        for id_wp in id_wps
    ]

    if flags.Input.isMC and \
       flags.Analysis.Muon.do_IFF_decoration:
        muon_branches.variables += [
            ("IFFClass_%SYS%", None),
            ("truthType", None),
            ("truthOrigin", None)
        ]

    if flags.Input.isMC and mu_output_flags.truth_parent_info:
        muon_branches.variables += get_TopHiggs_muon_truth_labels(flags)

    if flags.Input.isMC:
        muon_branches.variables += [
            (f"effSF_{id_wp}_%SYS%", None)
            for id_wp in id_wps
        ]

    # Requires MuonSelectorAlg to be run
    if mu_output_flags.run_selection:
        muon_branches.variables += [
            ("isAnalysisMuon_%SYS%", None)
        ]
        for index in range(flags.Analysis.Muon.amount):
            muon_branches.variables += [
                (f"isMuon{index+1}_%SYS%", None)
            ]
        for index in range(flags.Analysis.Lepton.amount):
            muon_branches.variables += [
                (f"isLepton{index+1}_%SYS%", None)
            ]

    muon_branches.variables += mu_output_flags.extra_variables
    if flags.Input.isMC:
        muon_branches.variables += mu_output_flags.mc_extra_variables

    return muon_branches.get_output_list()
