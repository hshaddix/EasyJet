def validate_hhh6b_analysis_prerequisites(flags):
    assert not flags.Analysis.disable_calib

    if flags.Analysis.do_resolved_trihiggs:
        assert flags.Analysis.do_small_R_jets

    if flags.Analysis.do_boosted_trihiggs:
        assert flags.Analysis.do_large_R_UFO_jets

    if flags.Analysis.do_resolved_trigger_SF:
        assert flags.Analysis.do_small_R_jets

    if flags.Analysis.do_resolved_trigger_bucket:
        assert flags.Analysis.do_small_R_jets
