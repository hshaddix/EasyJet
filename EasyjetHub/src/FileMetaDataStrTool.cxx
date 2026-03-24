/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "FileMetaDataStrTool.h"

#include "xAODMetaData/FileMetaData.h"
#include "xAODMetaData/FileMetaDataAuxInfo.h"

namespace Easyjet {

StatusCode FileMetaDataStrTool::initialize() {
    ATH_CHECK(AthAlgTool::initialize());
    ATH_CHECK(m_metaDataSvc.retrieve());
    return StatusCode::SUCCESS;
}

StatusCode FileMetaDataStrTool::postInitialize() {
    return StatusCode::SUCCESS;
}

StatusCode FileMetaDataStrTool::preExecute() {
    return StatusCode::SUCCESS;
}

StatusCode FileMetaDataStrTool::preStream() {
    return StatusCode::SUCCESS;
}

StatusCode FileMetaDataStrTool::postExecute() {
    return StatusCode::SUCCESS;
}

StatusCode FileMetaDataStrTool::preFinalize() {
    auto fmd = std::make_unique<xAOD::FileMetaData>();
    auto aux = std::make_unique<xAOD::FileMetaDataAuxInfo>();
    fmd->setStore(aux.get());
    if (!fmd->setValue(m_attrKey.value(), m_value.value())) {
        ATH_MSG_ERROR("Failed to set FileMetaData attribute '"
                      << m_attrKey.value() << "'");
        return StatusCode::FAILURE;
    }
    ATH_CHECK(m_metaDataSvc->record(
        std::move(fmd), m_outputKey.value()));
    ATH_CHECK(m_metaDataSvc->record(
        std::move(aux), m_outputKey.value() + "Aux."));
    return StatusCode::SUCCESS;
}

}  // namespace Easyjet
