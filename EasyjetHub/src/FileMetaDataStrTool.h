/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef EASYJET_FILEMETADATASTRTOOL_H
#define EASYJET_FILEMETADATASTRTOOL_H

#include "Gaudi/Property.h"
#include "GaudiKernel/extends.h"
#include "GaudiKernel/ServiceHandle.h"
#include "AthenaBaseComps/AthAlgTool.h"
#include "AthenaKernel/IAthenaOutputTool.h"
#include "AthenaKernel/IAthMetaDataSvc.h"

namespace Easyjet {

/// Tool that writes a single string key/value into xAOD::FileMetaData.
///
/// Generic IAthenaOutputTool; JSON encoding is done on the Python side.
class FileMetaDataStrTool
    : public extends<AthAlgTool, IAthenaOutputTool> {
public:
    using extends::extends;

    StatusCode initialize()     override;
    StatusCode postInitialize() override;
    StatusCode preExecute()     override;
    StatusCode preStream()      override;
    StatusCode postExecute()    override;
    StatusCode preFinalize()    override;

private:
    Gaudi::Property<std::string> m_outputKey{
        this, "OutputKey", "FileMetaData",
        "SG key for the FileMetaData container"};
    Gaudi::Property<std::string> m_attrKey{
        this, "AttrKey", "",
        "Attribute name inside FileMetaData"};
    Gaudi::Property<std::string> m_value{
        this, "Value", "",
        "String value to write"};

    ServiceHandle<IAthMetaDataSvc> m_metaDataSvc{
        "MetaDataSvc", name()};
};

}  // namespace Easyjet

#endif  // EASYJET_FILEMETADATASTRTOOL_H
