#ifndef SSWWANALYSIS_ENUMS
#define SSWWANALYSIS_ENUMS

namespace ssWWVBS
{

 enum Channel
  {
    SR,
    WZCR,
    misIDCR,
    incVR,
    LowDyVR,
    LowMjjVR,
    LowNjVR,
    tFakeVR,
    tEWKVR,
    lllVR,
    ZeeVR,
    prescale_DijetsCR,
    unprescale_DijetsCR,
  };

  enum TriggerChannel
  {
    SLT,
    prescaleSLT
  };

  enum Var {
    ele = 0,
    mu = 1,
    leadingele = 2,
    leadingmu = 3,
    subleadingele = 4,
    subleadingmu = 5,
  };

  enum Booleans
  {
    IS_ee,
    IS_mm,
    IS_em,
    IS_me,
    
    pass_trigger_SLT,
    pass_trigger_prescaleSLT,

    PASS_TRIGGER,
    PASS_TWO_LEPTONS,
    PASS_LEPTON_ID,
    EXACTLY_TWO_LEPTONS,
    TWO_SAME_CHARGE_LEPTONS,
    DILEPTON_MASS_THRESHOLD,
    DILEPTON_MASS_SIDEBAND_EE,
    MET,
    AT_LEAST_TWO_JETS,
    DIJETS_MASS_LOW,
    DIJETS_DELTA_RAPIDITY,
    BJET_VETO,
    DIJETS_MASS_HIGH,
    pass_SR,

    PASS_THREE_LEPTONS,
    EXACTLY_THREE_LEPTONS,
    pass_WZCR,
    pass_misIDCR,
    pass_incVR,
    pass_LowDyVR,
    pass_LowMjjVR,
    pass_LowNjVR,
    pass_tFakeVR,
    pass_tEWKVR,
    pass_lllVR,
    pass_ZeeVR,
    pass_prescale_DijetsCR,
    pass_unprescale_DijetsCR,

    EXACTLY_ONE_LEPTON,

    IS_ee_TRUTH,
    IS_mm_TRUTH,
    IS_em_TRUTH,

    EXACTLY_TWO_LEPTONS_TRUTH,
    SAME_CHARGE_LEPTONS_TRUTH,
    DILEPTON_MASS_TRUTH,

    MET_TRUTH,
    TWO_JETS_TRUTH,
    DIJET_MASS_TRUTH, 
    DIJET_RAPIDITY_TRUTH
  };

}

#endif