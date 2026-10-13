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

    extra_iso_suffixes = list(flags.Analysis.Muon.extra_iso_suffixes or [])

    if mu_output_flags.iso_variables:
        isolation_variables = [
            "neflowisol20",
            "topoetcone20",
            "ptvarcone30_Nonprompt_All_MaxWeightTTVA_pt500",
            "ptvarcone30_Nonprompt_All_MaxWeightTTVA_pt1000",
            "neflowisol20_CloseByCorr",
            "topoetcone20_CloseByCorr",
            "ptvarcone30_Nonprompt_All_MaxWeightTTVA_pt500_CloseByCorr",
            "ptvarcone30_Nonprompt_All_MaxWeightTTVA_pt1000_CloseByCorr",
        ]
        muon_branches.variables += [(name, None) for name in isolation_variables]

        all_suffixes = []
        if flags.Analysis.Muon.IsoDecSuffix:
            all_suffixes.append(flags.Analysis.Muon.IsoDecSuffix)
        all_suffixes += extra_iso_suffixes

        for suffix in all_suffixes:
            muon_branches.variables += [
                (f"{name}_{suffix}", None) for name in isolation_variables
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

    consider_extra_wp = any(len(wp) == 4 for wp in flags.Analysis.Muon.extra_wps)

    all_wps = [(flags.Analysis.Muon.ID, flags.Analysis.Muon.Iso)]
    if 'extra_wps' in flags.Analysis.Muon:
        all_wps += [(wp[0], wp[1]) for wp in flags.Analysis.Muon.extra_wps]

    id_wps = []
    for wp_idx, (id_, iso_) in enumerate(all_wps):
        id_iso_wp = id_ + "_" + iso_
        if wp_idx == 0 or not consider_extra_wp:
            ip_suffix = ""
        else:
            extra = flags.Analysis.Muon.extra_wps[wp_idx - 1]
            if len(extra) == 4:
                d0, z0 = extra[2], extra[3]
            else:
                d0 = flags.Analysis.Muon.maxD0Significance
                z0 = flags.Analysis.Muon.maxDeltaZ0SinTheta
            ip_suffix = ("_" + str(d0) + "_" + str(z0)).replace('.', 'p')
        id_wps.append(f"{id_iso_wp}{ip_suffix}")
        for s in extra_iso_suffixes:
            id_wps.append(f"{id_iso_wp}_{s}{ip_suffix}")

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
