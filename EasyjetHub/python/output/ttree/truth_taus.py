from EasyjetHub.output.ttree.branch_manager import BranchManager


def get_TopHiggs_tau_truth_labels(flags):
    truth_labels = []
    if not flags.Input.isPHYSLITE:
        truth_labels += [
            (f"parent{p}ParentsMask", "unsigned_long_long")
            for p in ["Higgs", "Z", "Top"]
        ]

    return truth_labels


def get_truthtau_branches(flags, input_container, output_prefix):
    truthtau_branches = BranchManager(
        input_container,
        output_prefix,
    )

    truthtau_branches.variables = [
        ("px", None),
        ("py", None),
        ("pz", None),
        ("e", None),
        ("m", None),
        ("numCharged", None),
        ("IsHadronicTau", None)
    ]
    truthtau_branches.variables += [
        (var + "_vis", None) for var in ["pt", "eta", "phi", "m"]
    ]
    truthtau_branches.variables += [
        ("decay_vertex_" + var, None) for var in ["x", "y", "z"]
    ]
    truthtau_branches.variables += [
        ("classifierParticleOrigin", None),
        ("classifierParticleType", None)
    ]

    return truthtau_branches.get_output_list()
