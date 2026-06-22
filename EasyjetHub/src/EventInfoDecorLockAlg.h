#ifndef EASYJET_EVENTINFODECORLOCKALG_H
#define EASYJET_EVENTINFODECORLOCKALG_H

#include <AthenaBaseComps/AthAlgorithm.h>
#include <StoreGate/ReadHandleKey.h>
#include <xAODEventInfo/EventInfo.h>

namespace Easyjet {

/// Lock all dynamic EventInfo decorations before pool output.
///
/// CP::SysWriteDecorHandle writes decorations via SG::Decorator<T>,
/// which does not auto-lock them the way SG::WriteDecorHandle does.
/// AthenaOutputStream's copyAuxStoreThinned warns about unlocked
/// decorations; this algorithm locks all of them explicitly so the
/// warnings are suppressed for the xAOD pool output stream.
class EventInfoDecorLockAlg final : public AthAlgorithm {
public:
    using AthAlgorithm::AthAlgorithm;
    StatusCode initialize() override;
    StatusCode execute(const EventContext& ctx) override;

private:
    SG::ReadHandleKey<xAOD::EventInfo> m_eventInfoKey{
        this, "EventInfoKey", "EventInfo", "EventInfo key"};
};

} // namespace Easyjet

#endif // EASYJET_EVENTINFODECORLOCKALG_H
