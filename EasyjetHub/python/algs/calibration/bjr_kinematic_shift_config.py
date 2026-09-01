# Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory


def BJRKinematicShiftSystCfg(flags, **kwargs):
    """Schedule Easyjet::BJRKinematicShiftSystAlg.

    The shift table defaults live on the algorithm itself (massShifts /
    ptShifts in BJRKinematicShiftSystAlg.h) -- pass them as kwargs to override.

    Must be scheduled after BaselineVarsZbbjCalibAlg and before the
    ZbbHistogramAlg instances; the caller (ZbbjCalib_cfg) owns that ordering.
    """
    cfg = ComponentAccumulator()
    cfg.addEventAlgo(
        CompFactory.Easyjet.BJRKinematicShiftSystAlg(
            "BJRKinematicShiftSystAlg",
            event="EventInfo",
            **kwargs,
        )
    )
    return cfg
