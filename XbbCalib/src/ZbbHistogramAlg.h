/*
  Copyright (C) 2002-2025 CERN for the benefit of the ATLAS collaboration
*/
#ifndef XBBCALIB_ZBBHISTOGRAMALG_H
#define XBBCALIB_ZBBHISTOGRAMALG_H

#include <AthenaBaseComps/AthAlgorithm.h>
#include <SystematicsHandles/SysListHandle.h>
#include <SystematicsHandles/SysReadHandle.h>
#include <SystematicsHandles/SysReadDecorHandle.h>
#include <HDF5Utils/IH5GroupSvc.h>
#include <xAODEventInfo/EventInfo.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Forward-declare H5::Group (avoid pulling in H5Cpp.h from the header)
namespace H5 { class Group; }

// PIMPL interface: concrete type (ZbbHist) lives in the .cxx anonymous
// namespace and holds the Hist4D; this interface avoids exposing the
// boost::histogram type in the header.
struct IZbbHist {
  virtual void fill(float mass, float pt, int wp_idx, int sys_idx,
                    float weight) = 0;
  virtual void write(H5::Group& parent,
                     const std::string& name,
                     const std::vector<std::string>& wp_labels,
                     const std::vector<std::string>& sys_labels) const = 0;
  virtual ~IZbbHist() = default;
};

namespace XBBCALIB {

class ZbbHistogramAlg final : public AthAlgorithm {
public:
  ZbbHistogramAlg(const std::string& name, ISvcLocator* pSvcLocator);
  ~ZbbHistogramAlg();
  StatusCode initialize() override;
  StatusCode execute(const EventContext& ctx) override;
  StatusCode finalize() override;

private:
  CP::SysListHandle m_systematicsList{this};
  CP::SysReadHandle<xAOD::EventInfo>
    m_eventHandle{this, "event", "EventInfo", "EventInfo container"};

  Gaudi::Property<std::string> m_weightDecoName{
    this, "weightDecoration", "",
    "EventInfo float deco for pre-computed event weight (must contain %SYS%)"};
  Gaudi::Property<std::string> m_selectionDecoName{
    this, "selectionDecoration", "",
    "EventInfo char deco: fill only when non-zero (empty = no cut)"};
  Gaudi::Property<std::string> m_massVarName{
    this, "massVar", "Zcand_0_m",
    "EventInfo float deco for jet mass (_%SYS% appended internally)"};
  Gaudi::Property<std::string> m_ptVarName{
    this, "ptVar", "Zcand_0_pt",
    "EventInfo float deco for jet pT (_%SYS% appended internally)"};
  Gaudi::Property<std::vector<std::string>> m_wpDecoNames{
    this, "wpDecorations", {},
    "EventInfo int decos for WP pass/fail, one per WP "
    "(_%SYS% appended internally; use empty string \"\" for a notag slot)"};

  // Mass axis: regular binning
  Gaudi::Property<int>   m_massBins {this, "massBins",  50};
  Gaudi::Property<float> m_massLower{this, "massLower", 50000.f};
  Gaudi::Property<float> m_massUpper{this, "massUpper", 150000.f};
  // pT axis: variable bin edges (MeV)
  Gaudi::Property<std::vector<float>> m_ptEdges{
    this, "ptEdges", {},
    "Bin edges for the variable pT axis in MeV (>= 2 entries required)"};

  Gaudi::Property<std::string> m_groupName{
    this, "groupName", "multidim",
    "HDF5 subgroup for output (empty = write at file root)"};
  Gaudi::Property<std::string> m_histName{
    this, "histName", "mass_pt_wp_sys",
    "Name of the histogram dataset written inside groupName"};

  ServiceHandle<IH5GroupSvc> m_output_svc{this, "output", ""};

  // Created in initialize():
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_weightHandle;
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_massHandle;
  std::unique_ptr<CP::SysReadDecorHandle<float>> m_ptHandle;
  std::shared_ptr<CP::SysReadDecorHandle<char>>  m_selHandle;
  std::vector<std::pair<int,
    std::unique_ptr<CP::SysReadDecorHandle<int>>>> m_wpHandles;

  // Histogram (PIMPL -- concrete type lives in .cxx)
  std::unique_ptr<IZbbHist> m_hist;

  // Labels for finalize()
  std::vector<std::string> m_wpLabels;   // "notag" + wp deco names
  std::vector<std::string> m_sysLabels;  // sys.name() or "NOSYS"
  std::vector<std::pair<int, CP::SystematicSet>> m_sysPairs;
};

} // namespace XBBCALIB
#endif
