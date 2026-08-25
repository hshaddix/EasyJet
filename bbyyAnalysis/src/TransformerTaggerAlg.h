/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#ifndef HHBBYYANALYSIS_TRANSFORMERTAGGERALG
#define HHBBYYANALYSIS_TRANSFORMERTAGGERALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysWriteDecorHandle.h>

#include <xAODEgamma/PhotonContainer.h>
#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

#include "ONNXInferenceTool/ONNXInferenceTool.h"

#include <AsgTools/ToolHandle.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace HHBBYY
{
  class TransformerTaggerAlg final : public AthHistogramAlgorithm
  {
  public:
    TransformerTaggerAlg(const std::string &name, ISvcLocator *pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

  private:
    CP::SysListHandle m_systematicsList {this};

    CP::SysReadHandle<xAOD::JetContainer>
      m_jetHandle {this, "jets", "bbyyAnalysisJets_%SYS%", "Jet container to read"};

    CP::SysReadHandle<xAOD::PhotonContainer>
      m_photonHandle {this, "photons", "bbyyAnalysisPhotons_%SYS%", "Photon container to read"};

    CP::SysReadHandle<xAOD::EventInfo>
      m_eventHandle {this, "event", "EventInfo", "EventInfo container to read"};

    CP::SysReadDecorHandle<char>
      m_isBtag {this, "bTagWPDecorName", "", "Name of input decorator for b-tagging"};

    CP::SysReadDecorHandle<int>
      m_PCBT {this, "PCBTDecorName", "", "Name of pseudo-continuous b-tagging decorator"};

    ToolHandle<ONNXInferenceTool> m_transformerTaggerTool
      {this, "TransformerTaggerTool", "", "Generic ONNX inference tool configured for the TransformerTagger model"};

    Gaudi::Property<bool> m_doSystematics
      {this, "doSystematics", false, "Run on all systematics"};

    Gaudi::Property<std::vector<std::string>> m_floatVariables
      {this, "floatVariableList", {}, "Name list of float variables"};

    Gaudi::Property<std::vector<std::string>> m_intVariables
      {this, "intVariableList", {}, "Name list of integer variables"};

    std::unordered_map<std::string, CP::SysWriteDecorHandle<float>> m_Fbranches;
    std::unordered_map<std::string, CP::SysWriteDecorHandle<int>> m_Ibranches;

  };
}

#endif
