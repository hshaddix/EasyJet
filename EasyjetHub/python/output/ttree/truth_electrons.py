from EasyjetHub.output.ttree.branch_manager import BranchManager


def get_TopHiggs_ele_truth_labels(flags):
    truth_labels = []
    if not flags.Input.isPHYSLITE:
        truth_labels += [
            (f"parent{p}ParentsMask", "unsigned_long_long")
            for p in ["Higgs", "Z", "Top"]
        ]

    return truth_labels


def get_truthelectron_branches(flags, input_container, output_prefix):
    truthelectron_branches = BranchManager(
        input_container,
        output_prefix,
    )

    truthelectron_branches.variables = [
        (var, None) for var in ["px", "py", "pz", "e", "m", "pdgId"]
    ]

    return truthelectron_branches.get_output_list()
