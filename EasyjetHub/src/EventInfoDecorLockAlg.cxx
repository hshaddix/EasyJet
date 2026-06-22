#include "EventInfoDecorLockAlg.h"
#include <AthContainers/AuxVectorData.h>
#include <AthContainersInterfaces/IConstAuxStore.h>
#include <CxxUtils/checker_macros.h>

StatusCode Easyjet::EventInfoDecorLockAlg::initialize() {
    ATH_CHECK(m_eventInfoKey.initialize());
    return StatusCode::SUCCESS;
}

StatusCode Easyjet::EventInfoDecorLockAlg::execute(const EventContext& ctx) {
    SG::ReadHandle<xAOD::EventInfo> ei(m_eventInfoKey, ctx);    if (!ei.isValid()) {
        ATH_MSG_ERROR("Failed to retrieve EventInfo with key "
                      << m_eventInfoKey.key());
        return StatusCode::FAILURE;
    }

    // EventInfo is a standalone SG::AuxElement; container() returns
    // the backing AuxVectorData (typically xAOD::AuxInfoBase from the
    // input DAOD).
    const SG::AuxVectorData* avd = ei->container();
    if (!avd) return StatusCode::SUCCESS;

    const SG::IConstAuxStore* cstore = avd->getConstStore();
    if (!cstore) return StatusCode::SUCCESS;

    // Collect the unlocked decoration IDs, then lock each one.
    // The const-cast is safe here: we are the sole writer at this
    // point in the single-threaded event loop.
    SG::auxid_set_t decors = cstore->getDecorIDs();
    SG::AuxVectorData* avd_nc ATLAS_THREAD_SAFE =
        const_cast<SG::AuxVectorData*>(avd);
    for (SG::auxid_t id : decors) {
        avd_nc->lockDecoration(id);
    }
    return StatusCode::SUCCESS;
}
