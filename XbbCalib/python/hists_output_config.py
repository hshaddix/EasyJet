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


def _final_selection_cfg(flags, hists_cfg, deco_name):
    selection_config = hists_cfg.selection.as_mutable()
    algo_name = f"FinalSelectionAlg_{deco_name}"
    sel_kwargs = {}
    if not flags.Input.isMC and hists_cfg.blinding.enable:
        sel_kwargs['blindingVariable'] = str(hists_cfg.blinding.variable)
        sel_kwargs['blindingMin'] = float(hists_cfg.blinding.min)
        sel_kwargs['blindingMax'] = float(hists_cfg.blinding.max)
    cfg = ComponentAccumulator()
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.FinalSelectionAlg(
            algo_name,
            selectionConfig=json.dumps(
                {k: v for k, v in selection_config.items()}),
            selectionDecoration=deco_name,
            **sel_kwargs,
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

    h5_svc = CompFactory.H5FileSvc(path=str(out_path))
    cfg.addService(h5_svc)

    weight_deco = "XbbCalib_event_weight_%SYS%"
    weight_kwargs = {}
    if flags.Input.isMC:
        weight_kwargs['sysWeight'] = "generatorWeight_%SYS%"
        extra = list(hists_cfg.weights.all_runs)
        if flags.GeoModel.Run is LHCPeriod.Run2:
            extra += list(hists_cfg.weights.run2)
        weight_kwargs['extraWeights'] = extra
    cfg.addEventAlgo(
        CompFactory.XBBCALIB.FinalWeightAlg(
            "FinalWeightAlg",
            weightDecoration=weight_deco,
            **weight_kwargs,
        )
    )

    # Single notag selection (also needed by ZbbHistogramAlg).
    selection_flag = "XbbCalib_pass_%SYS%"
    cfg.merge(_final_selection_cfg(
        flags, hists_cfg,
        deco_name=selection_flag
    ))

    cfg.addEventAlgo(
        CompFactory.XBBCALIB.EventInfoHistsAlg(
            "EventInfoHistsAlg",
            variables=variables,
            variables2d=variables2d,
            variables2dXvar=variables2dXvar,
            variables2dYvar=variables2dYvar,
            weightDecoration=weight_deco,
            output=h5_svc,
            selectionDecoration=selection_flag,
            nominalOnly=True,
            groupName="notag",
        )
    )

    # multidim config: dict keyed by name. Each entry defines one
    # ZbbHistogramAlg (e.g. leading / subleading). The dict key becomes
    # the HDF5 group name.
    for name, md in hists_cfg.multidim.items():
        mass_var = str(md.mass_var)
        wp_decos = [""] + [f"{md.wp_prefix}_{wp}" for wp in tag_wps]

        cfg.addEventAlgo(
            CompFactory.XBBCALIB.ZbbHistogramAlg(
                f"ZbbHistogramAlg_{name}",
                event="EventInfo",
                weightDecoration=weight_deco,
                selectionDecoration=selection_flag,
                massVar=mass_var,
                ptVar=str(md.pt_var),
                truthPtVar=str(md.truth_pt_var if "truth_pt_var" in md else ""),
                massBins=int(md.mass_n_bins),
                massLower=float(md.mass_lower),
                massUpper=float(md.mass_upper),
                ptEdges=list(md.pt_edges),
                truthPtEdges=list(md.truth_pt_edges if "truth_pt_edges" in md else ""),
                wpDecorations=wp_decos,
                groupName=name,
                histName="mass_pt_wp_sys",
                output=h5_svc,
            )
        )

    if flags.Input.isMC and not hists_cfg.disable_metadata:
        cfg.merge(MetadataAlgCfg(flags, h5_output_hists=h5_svc))
    return cfg
