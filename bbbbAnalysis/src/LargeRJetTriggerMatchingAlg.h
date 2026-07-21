/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

  LargeRJetTriggerMatchingAlg:
  Decorates offline large-R jets with whether they are matched to an
  online (HLT) large-R jet trigger object. Which trigger chain(s) to
  check is selected per-event from triggersByYear using the event's
  "EventInfo.dataTakingYear" decoration (written by EventInfoGlobalAlg),
  since the boosted trigger menu changes across data-taking years.
  Relies on the "trigPassed_<trigger>" EventInfo decoration, which must
  already exist for every chain named in triggersByYear (written by the
  standard ASG trigger-decision config, driven by
  flags.Analysis.TriggerChainsDeco).
*/

#ifndef HH4BANALYSIS_LARGERJETTRIGGERMATCHINGALG
#define HH4BANALYSIS_LARGERJETTRIGGERMATCHINGALG

#include <AthenaBaseComps/AthHistogramAlgorithm.h>

#include <AsgDataHandles/ReadHandleKey.h>
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AthContainers/Decorator.h>

#include <TriggerMatchingTool/IMatchingTool.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

#include <map>
#include <string>
#include <vector>

namespace HH4B
{
  class LargeRJetTriggerMatchingAlg final : public AthHistogramAlgorithm
  {
  public:
    LargeRJetTriggerMatchingAlg(const std::string &name, ISvcLocator *pSvcLocator);

    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

  private:
    ToolHandle<Trig::IMatchingTool> m_matchingTool
      { this, "trigMatchingTool", "", "Trigger matching tool" };

    SG::ReadHandleKey<xAOD::EventInfo> m_eventHandle
      { this, "event", "EventInfo", "EventInfo to read" };

    SG::ReadHandleKey<xAOD::JetContainer> m_LRjetsIn
      { this, "jetsIn", "", "Large-R jet container to read" };

    // per-event data-taking year, written by EventInfoGlobalAlg
    SG::ReadDecorHandleKey<xAOD::EventInfo> m_yearKey;

    Gaudi::Property<std::map<std::string, std::vector<std::string>>> m_triggersByYear
      { this, "triggersByYear", {},
        "Map from data-taking year (as string) to the boosted LR-jet "
        "trigger chains to match for that year; any chain in the list "
        "passing counts as a match for that event" };

    Gaudi::Property<double> m_matchThreshold
      { this, "matchThreshold", .1, "deltaR threshold used for LR-jet trigger matching" };

    // subset of m_triggersByYear kept after skimming out chains with no large-R jet leg
    std::map<std::string, std::vector<std::string>> m_LRJetTriggersByYear;

    // one read handle per distinct trigger chain appearing anywhere in m_LRJetTriggersByYear
    std::unordered_map<std::string, SG::ReadDecorHandleKey<xAOD::EventInfo>> m_triggerPassDecoKeys;

    // one decorator per distinct trigger chain; SG::Decorator (not WriteDecorHandle)
    // since jetsIn is a view container and elements aren't owned by it directly
    std::unordered_map<std::string, SG::Decorator<char>> m_jetIsMatchedKeys;

    static std::string sanitize(const std::string& trig);
    // true if at least one leg of the chain is a large-R ("a10") jet leg
    static bool hasLargeRJetLeg(const std::string& trig);
  };
}

#endif
