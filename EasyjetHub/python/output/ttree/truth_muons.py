from EasyjetHub.output.ttree.branch_manager import BranchManager


def get_TopHiggs_muon_truth_labels(flags):
    truth_labels = []
    if not flags.Input.isPHYSLITE:
        truth_labels += [
            (f"parent{p}ParentsMask", "unsigned_long_long")
            for p in ["Higgs", "Z", "Top"]
        ]

    return truth_labels


def get_truthmuon_branches(flags, input_container, output_prefix):
    truthmuon_branches = BranchManager(
        input_container,
        output_prefix,
    )

    truthmuon_branches.variables = [
        (var, None)
        for var in ["px", "py", "pz", "e", "m", "pdgId"]
    ]

    return truthmuon_branches.get_output_list()
