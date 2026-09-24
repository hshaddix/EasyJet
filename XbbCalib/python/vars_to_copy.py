# List of lrj decorection to copy
from EasyjetHub.steering.sample_metadata import is_at_least


def lrj_gn3x_working_points(flags):
    wps = []
    selections = flags.Analysis.Large_R_jet.flat_mass_selections
    for tagger, tagger_spec in sorted(selections.items()):
        wps += [
            f"xbb_select_{tagger}_{x}" for x in tagger_spec["wps"]
        ]
    return wps


def lrj_floats_to_copy(flags):
    floats_to_copy = []
    if is_at_least(flags, "p7017"):
        by_tag = flags.Analysis.Large_R_jet.variables_in_ptag
        floats_to_copy += by_tag.p7017
    if is_at_least(flags, "p6697"):
        floats_to_copy += ["bJR10v01_mass", "bJR10v01_pt"]
    lr_opts = flags.Analysis.ttree_output.collection_options
    if lr_opts.large_R_jets.substructure_info:
        floats_to_copy += [
            "Tau1_wta", "Tau2_wta", "Tau3_wta",
            "ECF1", "ECF2", "ECF3",
            "Split12", "Split23",
        ]
    return floats_to_copy


def lrj_ints_to_copy(flags):
    ints_to_copy = lrj_gn3x_working_points(flags)
    if flags.Input.isMC:
        ints_to_copy += ["R10TruthLabel_R22v1"]
    return ints_to_copy


