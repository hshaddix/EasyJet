/*
  Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration

  Make histograms of EventInfo decorations across systematics.
*/

#ifndef XBBCALIB_EVENTINFOHISTSALG_H
#define XBBCALIB_EVENTINFOHISTSALG_H

#include "Gaudi/Parsers/Factory.h"
#include "HDF5Utils/IH5GroupSvc.h"
#include <AthenaBaseComps/AthReentrantAlgorithm.h>
#include <xAODEventInfo/EventInfo.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include "AthContainers/AuxElement.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// Forward-declare H5::Group so write() can be declared without pulling in H5Cpp.h
namespace H5 { class Group; }

// Minimal abstract interface for file-private EventInfoHists implementation.
// Defined here so unique_ptr destructor can be instantiated in the header.
struct IEventInfoHists {
  virtual void fill(const xAOD::EventInfo&, float weight) = 0;
  virtual void write(H5::Group&) const = 0;
  virtual ~IEventInfoHists() = default;
};

namespace HH4B {

struct HistsConfig {
  std::map<std::string, std::map<std::string, float>> vars1d;
  std::map<std::string, std::map<std::string, float>> vars2d;
  std::map<std::string, std::string> var2dXNames;
  std::map<std::string, std::string> var2dYNames;
};

class EventInfoHistsAlg final : public AthReentrantAlgorithm {
public:
  EventInfoHistsAlg(const std::string& name, ISvcLocator* pSvcLocator);
  ~EventInfoHistsAlg();

  StatusCode initialize() override;
  StatusCode execute(const EventContext& ctx) const override;
  StatusCode finalize() override;

private:
  CP::SysListHandle m_systematicsList{this};

  CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{this, "event", "EventInfo", "EventInfo container"};
  Gaudi::Property<std::string> m_eventWeightName
    {this, "eventWeight", "",
     "EventInfo decoration for event weight (must include %SYS%)"};
  // created in initialize()
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_eventWeightHandle;
  ServiceHandle<IH5GroupSvc> m_output_svc{this, "output", "", "output file service"};

  HistsConfig m_histsConfig;

  Gaudi::Property<std::string> m_selectionDecoName
    {this, "selectionDecoration", "",
     "if non-empty, only fill when this EventInfo char decoration is non-zero"};

  // Additional per-event weight decoration names (attr names on EventInfo,
  // no "EventInfo." prefix) multiplied with the base eventWeight
  Gaudi::Property<std::vector<std::string>>
    m_extraWeightNames{this, "extraWeights", {}};

  Gaudi::Property<std::string> m_groupName
    {this, "groupName", "",
     "HDF5 subgroup written under the file root (empty = write at root)"};

  Gaudi::Property<std::string> m_blindingVariable
    {this, "blindingVariable", "",
     "decoration base name for blinding variable (empty = no blinding)"};
  Gaudi::Property<float> m_blindingMin{this, "blindingMin", 0.f};
  Gaudi::Property<float> m_blindingMax{this, "blindingMax", 0.f};

  // created in initialize() when selectionDecoration is non-empty
  std::shared_ptr<CP::SysReadDecorHandle<char>> m_selectionDecoHandle;
  // created in initialize() when blindingVariable is non-empty
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_blindingHandle;
  // Created in initialize() from m_extraWeightNames
  std::vector<SG::AuxElement::ConstAccessor<float>> m_extraWeightAccs;

  using JHist = std::pair<CP::SystematicSet, std::unique_ptr<IEventInfoHists>>;
  std::vector<JHist> m_hists;
};

} // namespace HH4B
#endif
