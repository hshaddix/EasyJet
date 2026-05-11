from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

# DSIDs for which Z->cc truth classification is meaningful.
# Zqq: 700849 (Sh_2214_Zqq_ptZ_200), Zqqgamma: 700352 (mc20), 701287 (mc23)
# Zcc: 701358 (Sh_22_Zcc_pTZ_200), Zccgamma: 701359 (Sh_22_Zccgamma_pTZ_100)
_ZQQ_DSIDS = {700849, 700352, 701287}
_ZCC_DSIDS = {701358, 701359}
_ZQQ_ZCC_DSIDS = _ZQQ_DSIDS | _ZCC_DSIDS


def is_zqq_sample(flags) -> bool:
    """True iff the input is one of the Zqq(gamma) inclusive samples."""
    return flags.Input.isMC and flags.Input.MCChannelNumber in _ZQQ_DSIDS


def is_zqq_zcc_sample(flags) -> bool:
    """True iff the input is a Zqq(gamma) or Zcc(gamma) sample."""
    return flags.Input.isMC and flags.Input.MCChannelNumber in _ZQQ_ZCC_DSIDS


def ZDecayTruthLabelAlgCfg(flags):
    """Schedule ZDecayTruthLabelAlg only for Zqq/Zcc samples."""
    cfg = ComponentAccumulator()
    if is_zqq_zcc_sample(flags):
        cfg.addEventAlgo(
            CompFactory.XBBCALIB.ZDecayTruthLabelAlg("ZDecayTruthLabelAlg")
        )
    return cfg


def truth_z_decay_int_vars(flags):
    """Return ['z_is_cc'] only when the branch should be written to the ntuple.

    The branch is suppressed when remove_zcc_overlap is True, since all stored
    events will have z_is_cc==0 by construction in that case.
    """
    save_branch = not flags.Analysis.remove_zcc_overlap
    if save_branch and is_zqq_zcc_sample(flags):
        return ["z_is_cc"]
    return []
