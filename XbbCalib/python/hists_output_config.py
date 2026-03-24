import json
from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaConfiguration.Enums import LHCPeriod
from JetTagDerivationUtils.MetadataAlgConfig import MetadataAlgCfg


def _validate_variables(variables):
    for varname, binning in variables.items():
        for key in ("lower", "upper", "n_bins"):
            if key not in binning:
                raise ValueError(
                    f"Variable '{varname}' missing required key '{key}'")
        if not isinstance(binning["n_bins"], int):
            raise TypeError(
                f"Variable '{varname}': n_bins must be an integer, "
                f"got {type(binning['n_bins']).__name__}")


def _validate_variables2d(variables2d):
    for outname, spec in variables2d.items():
        for axis in ("x", "y"):
            if axis not in spec:
                raise ValueError(
                    f"2D variable '{outname}' missing axis '{axis}'")
            for key in ("variable", "lower", "upper", "n_bins"):
                if key not in spec[axis]:
                    raise ValueError(
                        f"2D variable '{outname}' axis '{axis}' "
                        f"missing key '{key}'")


def _final_selection_cfg(selection_config, deco_name, algo_name,
                          tag_decoration=""):
    thresholds = {k: v for k, v in selection_config.items()}
    cfg = ComponentAccumulator()
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.FinalSelectionAlg(
            algo_name,
            selectionConfig=json.dumps(thresholds),
            selectionDecoration=deco_name,
            tagDecoration=tag_decoration,
        )
    )
    return cfg


def hists_output_cfg(flags, hists_cfg, out_path, tag_wps):
    """FinalSelectionAlg (optional) + EventInfoHistsAlg + MetadataAlg.

    hists_cfg is the output.hists ConfigItem branch, containing:
      .variables        - histogram variable binning dict
      .selection        - optional selection thresholds (omit to skip)
      .disable_metadata - suppress MetadataAlg if True
    out_path is the HDF5 output file path (falsy => no-op).
    tag_wps: list of WP decoration names ([] for no-tag-WP hists).
    """
    cfg = ComponentAccumulator()
    if not out_path:
        return cfg

    variables = hists_cfg.variables.as_mutable()
    _validate_variables(variables)

    variables2d, variables2dXvar, variables2dYvar = {}, {}, {}
    if 'hists2d' in hists_cfg:
        raw = hists_cfg.hists2d.as_mutable()
        _validate_variables2d(raw)
        for outname, spec in raw.items():
            variables2dXvar[outname] = spec['x']['variable']
            variables2dYvar[outname] = spec['y']['variable']
            variables2d[outname] = {
                'x_lower':  float(spec['x']['lower']),
                'x_upper':  float(spec['x']['upper']),
                'x_n_bins': float(spec['x']['n_bins']),
                'y_lower':  float(spec['y']['lower']),
                'y_upper':  float(spec['y']['upper']),
                'y_n_bins': float(spec['y']['n_bins']),
            }

    extra_weights = list(hists_cfg.weights.all_runs)
    if flags.GeoModel.Run is LHCPeriod.Run2:
        extra_weights += list(hists_cfg.weights.run2)

    blinding_variable = ""
    blinding_min = 0.
    blinding_max = 0.
    if not flags.Input.isMC:
        blinding_variable = str(hists_cfg.blinding.variable)
        blinding_min = float(hists_cfg.blinding.min)
        blinding_max = float(hists_cfg.blinding.max)

    h5_svc = CompFactory.H5FileSvc(path=str(out_path))
    cfg.addService(h5_svc)

    sel = hists_cfg.selection.as_mutable()

    # WP loop: None = "notag" (base cuts only), rest apply tag cut too.
    all_wps = [None] + list(tag_wps)

    for wp in all_wps:
        wp_label = "notag" if wp is None else wp
        deco_name = f"XbbCalib_pass_{wp_label}_%SYS%"
        tag_deco = "" if wp is None else f"Zcand_0_{wp}_%SYS%"

        cfg.merge(_final_selection_cfg(
            sel, deco_name,
            algo_name=f"FinalSelectionAlg_{wp_label}",
            tag_decoration=tag_deco))

        cfg.addEventAlgo(
            CompFactory.HH4B.EventInfoHistsAlg(
                name=f"EventInfoHistsAlg_{wp_label}",
                variables=variables,
                variables2d=variables2d,
                variables2dXvar=variables2dXvar,
                variables2dYvar=variables2dYvar,
                eventWeight="generatorWeight_%SYS%",
                extraWeights=extra_weights,
                output=h5_svc,
                selectionDecoration=deco_name,
                groupName=wp_label,
                blindingVariable=blinding_variable,
                blindingMin=blinding_min,
                blindingMax=blinding_max,
            )
        )

    if not hists_cfg.disable_metadata:
        cfg.merge(MetadataAlgCfg(flags, h5_output_hists=h5_svc))
    return cfg
