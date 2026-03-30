from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory

TAGGER_CONFIGS = {
    "BinaryHWW": {
        "onnx_file": "bbVVAnalysis/LundNet_Binary_QCD_HH4W_40_600_flatpt.onnx",
        "prefix": "BinaryHWW",
        "batch_size": 1
    },
    "BinaryHWW_FlatMass": {
        "onnx_file": "bbVVAnalysis/LundNet_Binary_QCD_HH4W_40_600_flatmass.onnx",
        "prefix": "BinaryHWW_FlatMass",
        "batch_size": 1
    },
    "NProng_NoHWW_FlatMass_Ver1": {
        "onnx_file": "bbVVAnalysis/LundNet_FourClass_NProng_NoHWW_2dFlat_Ver1.onnx",
        "prefix": "NProng_NoHWW_FlatMass_Ver1",
        "batch_size": 1
    },
    "NProng_S300_FlatPt_Ver2": {
        "onnx_file": "bbVVAnalysis/LundNet_FourClass_NProng_S300_FlatPt_Ver2.onnx",
        "prefix": "NProng_S300_FlatPt_Ver2",
        "batch_size": 1
    },
}


def lundnettagger_cfg(flags, lundnet_type, **kwargs):
    cfg = ComponentAccumulator()

    jetcoll = flags.Analysis.container_names.input.reco10UFOJet
    tagger_config = TAGGER_CONFIGS.get(lundnet_type)

    kwargs.setdefault("InputJetContainer", jetcoll)
    kwargs.setdefault("ModelPath", tagger_config["onnx_file"])
    kwargs.setdefault("Prefix", "")
    kwargs.setdefault("ScoreDecoration", tagger_config["prefix"])
    kwargs.setdefault("ExpectedBatchSize", tagger_config["batch_size"])

    cfg.setPrivateTools(
        CompFactory.LundNetTagger(
            f"LundNetTagger_{lundnet_type}",
            **kwargs
        )
    )

    return cfg


def lundnet_decor_cfg(flags, **kwargs):
    cfg = ComponentAccumulator()

    jetcoll = flags.Analysis.container_names.input.reco10UFOJet

    # First, need to decorate the jet with Lund variables
    cfg.addEventAlgo(
        CompFactory.CP.JetDecoratorAlg(
            name="LundVariablesToolAlg",
            jets=jetcoll,
            decorator=CompFactory.LundVariablesTool(
                name="LundVariablesTool"
            ),
            **kwargs,
        )
    )

    lundnet_type_list = list(flags.Analysis.Large_R_jet.lundnet_type)
    for lundnet_type in lundnet_type_list:
        print("lundnet_type:", lundnet_type)

        # JetDecoratorAlg to decorate the scores
        # LundNetTagger decorations are created as follows
        # {prefix}{ScoreDecoration}{LundNetScoreP1}
        cfg.addEventAlgo(
            CompFactory.CP.JetDecoratorAlg(
                name=f"LundNetTagger_{lundnet_type}_Alg",
                jets=jetcoll,
                decorator=cfg.popToolsAndMerge(lundnettagger_cfg(flags, lundnet_type)),
                **kwargs,
            )
        )

    return cfg
