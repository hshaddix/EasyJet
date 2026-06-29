/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/

#include "EventInfoHistsAlg.h"

#include <SystematicsHandles/SysReadDecorHandle.h>
#include "H5Cpp.h"
#include "HDF5Utils/histogram.h"
#include "boost/histogram.hpp"
#include <stdexcept>
#include <cmath>

namespace {  // anonymous — file-private

namespace bh = boost::histogram;
using WS = bh::accumulators::weighted_sum<float>;
using Hist = decltype(bh::make_histogram_with(
    bh::dense_storage<WS>(),
    bh::axis::regular<float>(1, 0.f, 1.f, "")));
using Hist2D = decltype(bh::make_histogram_with(
    bh::dense_storage<WS>(),
    bh::axis::regular<float>(1, 0.f, 1.f, ""),
    bh::axis::regular<float>(1, 0.f, 1.f, "")));
using HandleMap = std::map<std::string,
                           std::shared_ptr<CP::SysReadDecorHandle<float>>>;

void validate_config(
    const std::map<std::string, std::map<std::string, float>>& config)
{
  for (const auto& [varname, binning] : config) {
    for (const char* key : {"lower", "upper", "n_bins"}) {
      if (!binning.contains(key))
        throw std::invalid_argument(
          "EventInfoHistsAlg: variable '" + varname
          + "' missing required key '" + key + "'");
    }
    float n_bins_f = binning.at("n_bins");
    if (n_bins_f != std::floor(n_bins_f) || n_bins_f <= 0)
      throw std::invalid_argument(
        "EventInfoHistsAlg: variable '" + varname
        + "': n_bins must be a positive integer, got "
        + std::to_string(n_bins_f));
  }
}

void validate_config_2d(
    const std::map<std::string, std::map<std::string, float>>& config,
    const std::map<std::string, std::string>& xnames,
    const std::map<std::string, std::string>& ynames)
{
  for (const auto& [outname, binning] : config) {
    for (const char* key : {"x_lower", "x_upper", "x_n_bins",
                            "y_lower", "y_upper", "y_n_bins"}) {
      if (!binning.contains(key))
        throw std::invalid_argument(
          "EventInfoHistsAlg 2D: '" + outname
          + "' missing required key '" + key + "'");
    }
    for (const char* axis : {"x_n_bins", "y_n_bins"}) {
      float n = binning.at(axis);
      if (n != std::floor(n) || n <= 0)
        throw std::invalid_argument(
          "EventInfoHistsAlg 2D: '" + outname + "': " + axis
          + " must be a positive integer, got " + std::to_string(n));
    }
    if (!xnames.contains(outname))
      throw std::invalid_argument(
        "EventInfoHistsAlg 2D: '" + outname
        + "' has no entry in variables2dXvar");
    if (!ynames.contains(outname))
      throw std::invalid_argument(
        "EventInfoHistsAlg 2D: '" + outname
        + "' has no entry in variables2dYvar");
  }
}

struct HistVar {
  std::shared_ptr<CP::SysReadDecorHandle<float>> handle;
  std::string output_name;
  Hist histogram;
};

struct HistVar2D {
  std::shared_ptr<CP::SysReadDecorHandle<float>> x_handle;
  std::shared_ptr<CP::SysReadDecorHandle<float>> y_handle;
  std::string output_name;
  Hist2D histogram;
};

class EventInfoHists final : public IEventInfoHists {
public:
  EventInfoHists(
    const CP::SystematicSet& sys,
    const HandleMap& handles,
    const XBBCALIB::HistsConfig& cfg)
    : m_sys(&sys)
  {
    for (const auto& [varname, b] : cfg.vars1d) {
      m_vars.push_back({
        handles.at(varname),
        varname,
        bh::make_histogram_with(
          bh::dense_storage<WS>(),
          bh::axis::regular<float>(
            static_cast<int>(b.at("n_bins")),
            b.at("lower"), b.at("upper"), varname))
      });
    }
    for (const auto& [outname, b] : cfg.vars2d) {
      m_vars2d.push_back({
        handles.at(cfg.var2dXNames.at(outname)),
        handles.at(cfg.var2dYNames.at(outname)),
        outname,
        bh::make_histogram_with(
          bh::dense_storage<WS>(),
          bh::axis::regular<float>(
            static_cast<int>(b.at("x_n_bins")),
            b.at("x_lower"), b.at("x_upper"),
            cfg.var2dXNames.at(outname)),
          bh::axis::regular<float>(
            static_cast<int>(b.at("y_n_bins")),
            b.at("y_lower"), b.at("y_upper"),
            cfg.var2dYNames.at(outname)))
      });
    }
  }

  void fill(const xAOD::EventInfo& info, float weight) override {
    for (auto& v : m_vars)
      v.histogram(v.handle->get(info, *m_sys), bh::weight(weight));
    for (auto& v : m_vars2d)
      v.histogram(
        v.x_handle->get(info, *m_sys),
        v.y_handle->get(info, *m_sys),
        bh::weight(weight));
  }

  void write(H5::Group& group) const override {
    for (const auto& v : m_vars)
      H5Utils::hist::write_hist_to_group(group, v.histogram, v.output_name);
    for (const auto& v : m_vars2d)
      H5Utils::hist::write_hist_to_group(group, v.histogram, v.output_name);
  }

private:
  const CP::SystematicSet* m_sys;
  std::vector<HistVar> m_vars;
  std::vector<HistVar2D> m_vars2d;
};

} // anonymous namespace


namespace XBBCALIB {

EventInfoHistsAlg::EventInfoHistsAlg(const std::string& name,
                                     ISvcLocator* pSvcLocator)
  : AthReentrantAlgorithm(name, pSvcLocator)
{
  declareProperty("variables",       m_histsConfig.vars1d);
  declareProperty("variables2d",     m_histsConfig.vars2d);
  declareProperty("variables2dXvar", m_histsConfig.var2dXNames);
  declareProperty("variables2dYvar", m_histsConfig.var2dYNames);
}
EventInfoHistsAlg::~EventInfoHistsAlg() = default;

StatusCode EventInfoHistsAlg::initialize() {
  // Validate config early (throws std::invalid_argument on error).
  validate_config(m_histsConfig.vars1d);
  validate_config_2d(m_histsConfig.vars2d,
                     m_histsConfig.var2dXNames,
                     m_histsConfig.var2dYNames);

  // m_eventHandle must be initialized before the per-variable decor handles
  ATH_CHECK(m_eventHandle.initialize(m_systematicsList));
  if (!m_weightDecoName.value().empty()) {
    m_weightHandle = std::make_unique<CP::SysReadDecorHandle<float>>(
        m_weightDecoName.value(), this);
    ATH_CHECK(m_weightHandle->initialize(m_systematicsList, m_eventHandle));
  }

  // Build shared handles (one per variable).  Use the non-property constructor
  // so we can create them dynamically here in initialize().
  HandleMap var_handles;
  for (const auto& [varname, _] : m_histsConfig.vars1d) {
    var_handles[varname] = std::make_shared<CP::SysReadDecorHandle<float>>(
      varname + "_%SYS%", this);
    ATH_CHECK(var_handles[varname]->initialize(m_systematicsList,
                                               m_eventHandle));
  }
  // Add handles for 2D x/y variables (skip if already registered above).
  for (const auto& [outname, _] : m_histsConfig.vars2d) {
    for (const std::string& vname : {
           m_histsConfig.var2dXNames.at(outname),
           m_histsConfig.var2dYNames.at(outname)}) {
      if (!var_handles.contains(vname)) {
        var_handles[vname] =
          std::make_shared<CP::SysReadDecorHandle<float>>(
            vname + "_%SYS%", this);
        ATH_CHECK(var_handles[vname]->initialize(m_systematicsList,
                                                 m_eventHandle));
      }
    }
  }
  if (!m_selectionDecoName.value().empty()) {
    m_selectionDecoHandle = std::make_shared<CP::SysReadDecorHandle<char>>(
      m_selectionDecoName.value(), this);
    ATH_CHECK(m_selectionDecoHandle->initialize(m_systematicsList,
                                                m_eventHandle));
  }

  ATH_CHECK(m_output_svc.retrieve());
  ATH_CHECK(m_systematicsList.initialize());

  // Create one EventInfoHists per systematic; handles are shared via shared_ptr.
  for (const auto& sys : m_systematicsList.systematicsVector()) {
    ATH_MSG_DEBUG("booking " << sys.name());
    m_hists.emplace_back(sys,
      std::make_unique<EventInfoHists>(sys, var_handles, m_histsConfig));
  }
  return StatusCode::SUCCESS;
}

StatusCode EventInfoHistsAlg::execute(const EventContext& ctx) const {
  (void)ctx;
  for (auto& [sys, hists] : m_hists) {
    if (m_nominalOnly && !sys.empty()) continue;
    ATH_MSG_VERBOSE("filling " << sys.name());
    const xAOD::EventInfo* evtInfo = nullptr;
    ATH_CHECK(m_eventHandle.retrieve(evtInfo, sys, ctx));
    if (m_selectionDecoHandle
        && !m_selectionDecoHandle->get(*evtInfo, sys)) continue;
    float weight = m_weightHandle
        ? m_weightHandle->get(*evtInfo, sys) : 1.f;
    hists->fill(*evtInfo, weight);
  }
  return StatusCode::SUCCESS;
}

StatusCode EventInfoHistsAlg::finalize() {
  ATH_MSG_DEBUG("finalizing");
  // Explicitly open root group — H5File::getLocId() returns a file ID which
  // behaves differently from a group ID in H5Lexists.
  // Note that this should be removed after atlas/athena!86879
  H5::Group root_grp = m_output_svc->group()->openGroup("/");
  H5::Group wp_group;
  H5::Group* parent = &root_grp;
  if (!m_groupName.value().empty()) {
    const std::string& gn = m_groupName.value();
    if (!H5Lexists(root_grp.getId(), gn.c_str(), H5P_DEFAULT))
      root_grp.createGroup(gn);
    wp_group = root_grp.openGroup(gn);
    parent = &wp_group;
  }
  for (const auto& [sys, hists] : m_hists) {
    if (m_nominalOnly && !sys.empty()) continue;
    std::string name = sys.name().empty() ? "nominal" : sys.name();
    ATH_MSG_DEBUG("saving " << name);
    if (!H5Lexists(parent->getId(), name.c_str(), H5P_DEFAULT))
      parent->createGroup(name);
    H5::Group sysgroup = parent->openGroup(name);
    hists->write(sysgroup);
  }
  return StatusCode::SUCCESS;
}

} // namespace XBBCALIB
