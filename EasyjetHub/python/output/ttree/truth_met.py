from EasyjetHub.output.ttree.branch_manager import BranchManager


def get_truthmet_branches(flags, input_container, output_prefix):
    truthmet_branches = BranchManager(
        input_container,
        output_prefix,
    )

    truthmet_branches.variables = [
        (var, None)
        for var in ["mpx", "mpy", "sumet"]
    ]

    return truthmet_branches.get_output_list()
