from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from OutputStreamAthenaPool.OutputStreamConfig import OutputStreamCfg
from xAODMetaDataCnv.InfileMetaDataConfig import SetupMetaDataForStreamCfg


def container(typename, key, items=None):
    if items is None:
        items = []
    itemstring = '.'.join(items)
    return [
        f'xAOD::{typename}Container#{key}',
        f'xAOD::{typename}AuxContainer#{key}Aux.{itemstring}',
    ]


def get_xaod_cfg(flags, seqname):
    ca = ComponentAccumulator()
    AcceptAlgs = [seqname]
    extra_items = []
    for jetname in flags.Analysis.output.aod.containers.jets:
        extra_items += container("Jet", jetname)
    # Use the concrete EventInfoAuxContainer CLID so that decoration
    # locking is tracked correctly.  An empty variables list (write all)
    # becomes a trailing dot in the item string; a non-empty list is
    # appended as dot-separated names to restrict which dynamic branches
    # are written.
    variables = flags.Analysis.output.aod.event_info.variables
    itemstring = '.'.join(variables)
    ei_items = [
        "xAOD::EventInfo#EventInfo",
        f"xAOD::EventInfoAuxContainer#EventInfoAux.{itemstring}",
    ]
    item_list = ei_items + extra_items
    # Lock all EventInfo dynamic decorations before writing so that
    # copyAuxStoreThinned does not warn about unlocked decorations.
    # CP::SysWriteDecorHandle writes via SG::Decorator, which does not
    # auto-lock the way SG::WriteDecorHandle does.
    ca.addEventAlgo(
        CompFactory.Easyjet.EventInfoDecorLockAlg(
            name="EventInfoDecorLockAlg"))
    ca.merge(OutputStreamCfg(flags, "AOD", ItemList=item_list,
                             AcceptAlgs=AcceptAlgs))
    from AthenaConfiguration.Enums import MetadataCategory
    ca.merge(SetupMetaDataForStreamCfg(
        flags, streamName="AOD",
        AcceptAlgs=AcceptAlgs,
        createMetadata=[MetadataCategory.IOVMetaData]))
    return ca
