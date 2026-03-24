import json
from AthenaConfiguration.ComponentFactory import CompFactory
from OutputStreamAthenaPool.OutputStreamConfig import addToMetaData


def xbb_tag_meta_cfg(flags, tag_wps, accept_algs=None):
    """Write Xbb WP list as JSON into xAOD::FileMetaData.

    accept_algs: passed to addToMetaData as AcceptAlgs; must match
      the AcceptAlgs already set on the AOD output stream to avoid
      a merge conflict (e.g. ['XbbCalibSeq']).
    """
    wps_json = json.dumps(list(tag_wps))
    tool = CompFactory.Easyjet.FileMetaDataStrTool(
        "XbbTagMetaTool",
        OutputKey="FileMetaDataXbbCalib",
        AttrKey="xbbTagWpsJson",
        Value=wps_json,
    )
    kwargs = {}
    if accept_algs is not None:
        kwargs['AcceptAlgs'] = accept_algs
    return addToMetaData(
        flags, "AOD",
        itemOrList=[
            "xAOD::FileMetaData#FileMetaDataXbbCalib",
            "xAOD::FileMetaDataAuxInfo#FileMetaDataXbbCalibAux.",
        ],
        HelperTools=[tool],
        **kwargs,
    )
