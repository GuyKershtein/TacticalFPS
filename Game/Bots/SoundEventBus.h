#pragma once

#include "SoundEvent.h"
#include <vector>

namespace Game {

// A one-frame mailbox of sound events: whatever made noise this frame
// pushes to it, every bot's perception reads it, and GameApplication
// clears it at the end of the frame. Deliberately not a full 3D-audio
// propagation system (occlusion by walls, reverb, falloff curves) — that's
// Milestone 12's job; bots just need "was something loud nearby."
class SoundEventBus {
public:
    void Push(const SoundEvent& event) { m_events.push_back(event); }
    const std::vector<SoundEvent>& GetEvents() const { return m_events; }
    void Clear() { m_events.clear(); }

private:
    std::vector<SoundEvent> m_events;
};

} // namespace Game
