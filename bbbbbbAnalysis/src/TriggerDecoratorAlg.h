/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration

  TriggerDecoratorAlg:
  An alg that saves the event-level + tau-trigger matching info as decorations
  Required for antiTau definition in TauDecoratorAlg
*/


// Always protect against multiple includes!
#ifndef HHH6BANALYSIS_TRIGGERDECORATORALG
#define HHH6BANALYSIS_TRIGGERDECORATORALG

#include <AthenaBaseComps/AthReentrantAlgorithm.h>

#include <AsgDataHandles/ReadHandleKey.h>
#include <AsgDataHandles/ReadDecorHandleKey.h>
#include <AsgDataHandles/WriteDecorHandleKey.h>
#include <AsgDataHandles/ReadDecorHandle.h>
#include <AsgDataHandles/WriteDecorHandle.h>

#include <xAODEventInfo/EventInfo.h>
#include <xAODJet/JetContainer.h>

namespace HHH6B
{
  enum TriggerChannel
  {
    SINGB,
    DIB_SINGJ,
    DIB_HT,
    DIB_DIJ,
    SB,
    J75,
    J80,
  };

  enum TriggerType
  {
    L1,
    HLT
  };

  enum TriggerSFSystType
  {
    SF_NOSYS,
    SF_STAT,
    SF_SYST
  };
  
  class TriggerDecoratorAlg final : public AthReentrantAlgorithm
  {
    
  public:
    using AthReentrantAlgorithm::AthReentrantAlgorithm;

    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) const override;

  private:

    const std::unordered_map<HHH6B::TriggerChannel, std::string> m_triggerChannels =
      {
        {HHH6B::SINGB, "1b"},
        {HHH6B::DIB_SINGJ, "2b1j"},
        {HHH6B::DIB_HT, "2bHT"},
        {HHH6B::DIB_DIJ, "2b2j"},
        {HHH6B::J75, "j75"},
        {HHH6B::J80, "j80"}
      };

    SG::ReadHandleKey<xAOD::EventInfo> m_eventInfoKey
      { this, "event", "EventInfo", "EventInfo to read" };

    SG::ReadDecorHandleKey<xAOD::EventInfo> m_yearKey;

    SG::ReadHandleKey<xAOD::JetContainer> m_jetsKey
      { this, "jets", "", "Jet container" };

    Gaudi::Property<std::vector<std::string>> m_triggers
      { this, "triggerLists", {}, "Name list of trigger" };
    Gaudi::Property<bool> m_doL1SF{this, "doL1SF", false, "Include jet trigger L1 SF"};
    Gaudi::Property<bool> m_doHLTSF{this, "doHLTSF", false, "Include jet trigger HLT SF"};
    Gaudi::Property<bool> m_isMC { this, "isMC", false, "Is this simulation?" };
    std::unordered_map<std::string, SG::ReadDecorHandleKey<xAOD::EventInfo> >
      m_triggerdecoKeys;

    typedef std::pair<HHH6B::TriggerType, HHH6B::TriggerSFSystType> triggerSFType;
    typedef std::map<triggerSFType, std::unordered_map<std::string, SG::ReadDecorHandle<xAOD::EventInfo, float>>> triggerSFIndivReadDecoMap;

    std::map<triggerSFType, std::unordered_map<std::string, SG::ReadDecorHandleKey<xAOD::EventInfo> >> 
      m_triggerSFIndivDecoKeysCont;

    std::unordered_map<HHH6B::TriggerChannel, SG::WriteDecorHandleKey<xAOD::EventInfo> > 
      m_pass_DecorKey;

    std::vector<triggerSFType> m_triggerSFTypeList;

    std::string getSFDecorKeyName( const triggerSFType t) const;

    SG::WriteDecorHandleKey<xAOD::EventInfo> m_bucketDecoratorKey{"EventInfo.bucket"};
    std::map<triggerSFType, SG::WriteDecorHandleKey<xAOD::EventInfo>> m_trigSFDecoratorKeys;

    typedef std::unordered_map<std::string, SG::ReadDecorHandle<xAOD::EventInfo, bool> > trigReadDecoMap;
    typedef std::unordered_map<HHH6B::TriggerChannel, SG::WriteDecorHandle<xAOD::EventInfo, char> > passWriteDecoMap;
    typedef std::unordered_map<int, std::unordered_map<HHH6B::TriggerChannel, std::vector<std::string>>> triggerMap;

    void evaluateTriggerCuts(const xAOD::EventInfo* eventInfo, const std::vector<std::string> &Triggers,
                                                  const trigReadDecoMap& triggerdecos, passWriteDecoMap& pass_decos, 
                                                  HHH6B::TriggerChannel flag) const;

    void evaluateTriggerBuckets(const xAOD::EventInfo* eventInfo, 
                                const SG::ReadDecorHandle<xAOD::EventInfo, unsigned int>& year, 
                                SG::ReadHandle<xAOD::JetContainer> jets, 
                                const triggerSFIndivReadDecoMap& triggersf_decos,
                                passWriteDecoMap& pass_decos, 
                                SG::WriteDecorHandle<xAOD::EventInfo, int>& bucketDecorator,
                                std::map<triggerSFType, SG::WriteDecorHandle<xAOD::EventInfo, float>>& trigSFDecorators
        ) const;
    
    const triggerMap m_triggerMap = {
        {2016, {
            {HHH6B::SINGB, {
                "HLT_j225_bmv2c2060_split"
            }},
            {HHH6B::DIB_SINGJ, {
                "HLT_j100_2j55_bmv2c2060_split"
            }},
            {HHH6B::DIB_HT, {
                "HLT_2j35_bmv2c2060_split_2j35_L14J15.0ETA25"
            }},
            {HHH6B::DIB_DIJ, {
                "HLT_2j35_bmv2c2060_split_2j35_L14J15.0ETA25"
            }}
        }},
        {2017, {
            {HHH6B::SINGB, {
                "HLT_j225_gsc300_bmv2c1070_split"
            }},
            {HHH6B::DIB_SINGJ, {
                "HLT_j110_gsc150_boffperf_split_2j35_gsc55_bmv2c1070_split_L1J85_3J30"
            }},
            {HHH6B::DIB_HT, {
                "HLT_2j35_gsc55_bmv2c1050_split_ht300_L1HT190-J15s5.ETA21"
            }},
            {HHH6B::DIB_DIJ, {
                "HLT_2j15_gsc35_bmv2c1040_split_2j15_gsc35_boffperf_split_L14J15.0ETA25"
            }}
        }},
        {2018, {
            {HHH6B::SINGB, {
                "HLT_j225_gsc300_bmv2c1070_split"
            }},
            {HHH6B::DIB_SINGJ, {
                "HLT_j110_gsc150_boffperf_split_2j45_gsc55_bmv2c1070_split_L1J85_3J30"
            }},
            {HHH6B::DIB_HT, {
                "HLT_2j45_gsc55_bmv2c1050_split_ht300_L1HT190-J15s5.ETA21"
            }},
            {HHH6B::DIB_DIJ, {
                "HLT_2j35_bmv2c1060_split_2j35_L14J15.0ETA25"
            }}
        }},
        {2022, {
            {HHH6B::J80, {
                "HLT_j80c_020jvt_j55c_020jvt_j28c_020jvt_j20c_020jvt_SHARED_2j20c_020jvt_bdl1d77_pf_ftf_presel2c20XX2c20b85_L1J45p0ETA21_3J15p0ETA25"
            }}
        }},
        {2023, {
            {HHH6B::J75, {
                "HLT_j75c_020jvt_j50c_020jvt_j25c_020jvt_j20c_020jvt_SHARED_2j20c_020jvt_bgn177_pf_ftf_presel2c20XX2c20b85_L1J45p0ETA21_3J15p0ETA25"
            }},
            {HHH6B::J80, {
                "HLT_j80c_020jvt_j55c_020jvt_j28c_020jvt_j20c_020jvt_SHARED_2j20c_020jvt_bgn177_pf_ftf_presel2c20XX2c20b85_L1J45p0ETA21_3J15p0ETA25"
            }}
        }}
    };

    const std::vector<std::string>& getTriggerPaths(
          const triggerMap& trigger_map,
          int year,
          HHH6B::TriggerChannel triggerType) const;
    const std::vector<std::string>& getTriggerPaths(
          const triggerMap& trigger_map,
          int year,
          int bucket) const;
  };
}

#endif
