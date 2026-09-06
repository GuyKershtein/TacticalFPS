#pragma once

#include "SoundClip.h"
#include <unordered_map>

namespace Engine {

enum class SoundId {
    Gunshot,
    ReloadClick,
    FootstepConcrete,
    FootstepMetal,
    FootstepWood,
    ImpactConcrete,
    ImpactMetal,
    ImpactWood,
    Explosion,
    FlashRing,
    PlantBeep,
    DefuseBeep,
    DeathTone,
};

// Generates every clip once at startup and hands out references to the
// cached result — playing a sound never re-synthesizes it.
class SoundBank {
public:
    void Init();
    const SoundClip& Get(SoundId id) const;

private:
    std::unordered_map<SoundId, SoundClip> m_clips;
};

} // namespace Engine
