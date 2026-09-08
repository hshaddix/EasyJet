def get_fixed_btag_wps(flags):
    """
    Make sure btag wps are fixed-cut, continuous just used for decoration for c-tagging.
    """
    btag_wps = [flags.Analysis.Small_R_jet.btag_wp]
    btag_wps += flags.Analysis.Small_R_jet.btag_extra_wps

    btag_wps_fixed = [wp for wp in btag_wps if "_FixedCutBEff_" in wp]

    if not btag_wps_fixed:
        raise ValueError("No FixedCutBEff WPs requested.")

    return btag_wps_fixed


def get_basic_fourmom_variables():
    float_variable_names = []

    objects = []
    for object in objects:
        for var in ["m", "pt", "eta", "phi"]:
            float_variable_names.append(f"{object}_{var}")

    return float_variable_names
