from EasyjetHub.output.ttree.branch_manager import BranchManager, SystOption


def get_met_branches(flags, input_container, output_prefix):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

    met_branches = BranchManager(
        input_container,
        output_prefix,
        systematics_option=_syst_option,
        systematics_suffix_separator=flags.Analysis.systematics_suffix_separator
    )

    met_branches.variables += [
        ("met", None),
        ("phi", None),
        ("sumet", None)
    ]

    if flags.Analysis.Met.saveSignificance:
        met_branches.variables += [("significance_%SYS%", None)]

    return met_branches.get_output_list()


def get_metnet_branches(flags, input_container, output_prefix):
    _syst_option = SystOption.ALL_SYST
    if flags.Analysis.disable_calib:
        _syst_option = SystOption.NONE

    metnet_branches = BranchManager(
        input_container,
        output_prefix,
        systematics_option=_syst_option,
        systematics_suffix_separator=flags.Analysis.systematics_suffix_separator
    )

    # METNet only predicts the MET vector (no sumet)
    metnet_branches.variables += [
        ("met", None),
        ("phi", None)
    ]

    return metnet_branches.get_output_list()
