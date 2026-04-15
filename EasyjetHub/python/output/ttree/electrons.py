from EasyjetHub.output.ttree.branch_manager import BranchManager, SystOption
from EasyjetHub.output.ttree.truth_electrons import get_TopHiggs_ele_truth_labels


def get_electron_branches(flags, tree_flags, input_container, output_prefix):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

    el_output_flags = tree_flags.collection_options.electrons

    electron_branches = BranchManager(
        input_container,
        output_prefix,
        systematics_option=_syst_option,
        systematics_suffix_separator=flags.Analysis.systematics_suffix_separator
    )

    if tree_flags.slim_variables_with_syst:
        electron_branches.syst_only_for = ["pt"]

    electron_branches.add_four_mom_branches(do_mass=False)
    electron_branches.variables += [
        ("charge", None)
    ]

    if el_output_flags.iso_variables:
        electron_branches.variables += [
            ("topoetcone20", None),
            ("topoetcone20_CloseByCorr", None),
            ("ptvarcone30_Nonprompt_All_MaxWeightTTVALooseCone_pt1000", None)
        ]

    if el_output_flags.id_variables:
        if not flags.Input.isPHYSLITE:
            electron_branches.variables += [
                (f"DFCommonElectronsDNN_p{c}", None)
                for c in ["el", "cf", "pc", "hf", "le", "lh"]
            ]

    if flags.Analysis.Electron.do_reco_decoration:
        electron_branches.variables += [
            ("author", None)
        ]

    if flags.Analysis.do_overlap_removal:
        electron_branches.variables += [
            ("passesOR_%SYS%", None)
        ]

    electron_branches.variables += [
        ("d0_%SYS%", None),
        ("d0sig_%SYS%", None),
        ("z0sintheta_%SYS%", None),
        ("z0sinthetasig_%SYS%", None)
    ]

    if flags.Analysis.Electron.MergeLRT:
        electron_branches.variables += [
            ("isLRT", None)
        ]

    id_wps = [f'{flags.Analysis.Electron.ID}_{flags.Analysis.Electron.Iso}']

    if 'extra_wps' in flags.Analysis.Electron:
        for wp in flags.Analysis.Electron.extra_wps:
            id_wps.append(wp[0] + "_" + wp[1])

    electron_branches.variables += [
        (f"baselineSelection_{id_wp}_%SYS%", None)
        for id_wp in id_wps
    ]

    if flags.Analysis.Electron.do_track_decoration:
        electron_branches.variables += [
            ("mllConv", "float"),
            ("mllConvAtConvV", "float"),
            ("radiusConv", "float"),
            ("separationMinDCT", "float")
        ]
        if not flags.Input.isPHYSLITE:
            electron_branches.variables += [
                ("DFCommonAddAmbiguity", None),
                ("ambiguityType", None)
            ]

    if flags.Input.isMC and \
       flags.Analysis.Electron.do_IFF_decoration:
        electron_branches.variables += [
            ("IFFClass_%SYS%", "int"),
            ("truthType", None),
            ("truthOrigin", None),
            ("firstEgMotherTruthType", None),
            ("firstEgMotherTruthOrigin", None),
            ("firstEgMotherPdgId", None)
        ]

    if flags.Input.isMC and el_output_flags.truth_parent_info:
        electron_branches.variables += get_TopHiggs_ele_truth_labels(flags)

    if flags.Input.isMC:
        electron_branches.variables += [
            (f"effSF_{id_wp}_%SYS%", None)
            for id_wp in id_wps
            if not ("DNN" in id_wp or "NoPix" in id_wp)
        ]

    # Requires ElectronSelectorAlg to be run
    if el_output_flags.run_selection:
        electron_branches.variables += [
            ("isAnalysisElectron_%SYS%", None)
        ]
        for index in range(flags.Analysis.Electron.amount):
            electron_branches.variables += [
                (f"isElectron{index+1}_%SYS%", None)
            ]
        for index in range(flags.Analysis.Lepton.amount):
            electron_branches.variables += [
                (f"isLepton{index+1}_%SYS%", None)
            ]

    electron_branches.variables += el_output_flags.extra_variables
    if flags.Input.isMC:
        electron_branches.variables += el_output_flags.mc_extra_variables

    return electron_branches.get_output_list()
