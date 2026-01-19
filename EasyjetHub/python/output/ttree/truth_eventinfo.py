from EasyjetHub.output.ttree.branch_manager import BranchManager


def get_truth_event_info_branches(flags):

    truth_eventinfo_branches = BranchManager(
        input_container="TruthEvents",
        output_prefix="truth",
        variables=[
            ("PDFID1", None),
            ("PDFID2", None),
            ("PDGID1", None),
            ("PDGID2", None),
            ("Q", None),
            ("X1", None),
            ("X2", None)
        ]
    )

    return truth_eventinfo_branches.get_output_list()
