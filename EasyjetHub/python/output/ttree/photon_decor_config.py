from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.Enums import LHCPeriod

def photon_decor_config(flags, **kwargs):
    cfg = ComponentAccumulator()

    photoncoll = flags.Analysis.container_names.input.photons
    isRun3 = True if flags.GeoModel.Run is LHCPeriod.Run3 else False
    cfg.addEventAlgo(
        CompFactory.Easyjet.PhotonDecoratorAlg(
            f"PhotonDecor_{photoncoll}",
            photonsIn=photoncoll,
            isRun3=isRun3,
            isMC=flags.Input.isMC,
            doRetrieveConversionVariables=flags.Analysis.Photon.do_conversion_decoration,
            doRetrieveExtendedTruthInfos=flags.Analysis.Photon.do_extended_truth_decoration,
            **kwargs
        )
    )

    return cfg
