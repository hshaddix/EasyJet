from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

TAGGER_CONFIGS = {
    "DNNTaggerTopQuarkContained": {
        "calib_area": "Winter2024_R22_PreRecs/JSSWTopTaggerDNN/",
        "config_file": "DNNTagger_AntiKt10UFOSD_TopContained{wp}_Oct30.dat"
    },
    "DNNTaggerTopQuarkInclusive": {
        "calib_area": "Winter2024_R22_PreRecs/JSSWTopTaggerDNN/",
        "config_file": "DNNTagger_AntiKt10UFOSD_TopInclusive{wp}_Oct30.dat"
    }
}


def toptagger_cfg(flags, WP, **kwargs):
    cfg = ComponentAccumulator()

    jetcoll = flags.Analysis.container_names.input.reco10UFOJet
    toptag_type = flags.Analysis.Large_R_jet.toptag_type
    tagger_config = TAGGER_CONFIGS.get(toptag_type)

    kwargs.setdefault("ContainerName", jetcoll)
    kwargs.setdefault("ConfigFile", tagger_config["config_file"].format(wp=WP))
    kwargs.setdefault("CalibArea", tagger_config["calib_area"])
    kwargs.setdefault("IsMC", flags.Input.isMC)

    cfg.setPrivateTools(CompFactory.JSSWTopTaggerDNN(**kwargs))
    return cfg


def toptag_decor_cfg(flags, **kwargs):
    cfg = ComponentAccumulator()

    jetcoll = flags.Analysis.container_names.input.reco10UFOJet
    toptag_wp = flags.Analysis.Large_R_jet.toptag_wp
    toptag_extra_wps = list(flags.Analysis.Large_R_jet.toptag_extra_wps or [])

    for wp in [toptag_wp] + toptag_extra_wps:
        cfg.addEventAlgo(
            CompFactory.Easyjet.TopTaggingDecoratorAlg(
                f"TopTagDecor{wp}_{jetcoll}",
                jetsIn=jetcoll,
                Toptagger=cfg.popToolsAndMerge(toptagger_cfg(flags, wp)),
                **kwargs,
            )
        )

    return cfg
