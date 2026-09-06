#include "SoundBank.h"
#include "ProceduralSoundGenerator.h"

#include <cstdio>

namespace Engine {

void SoundBank::Init() {
    using namespace ProceduralSound;
    m_clips[SoundId::Gunshot] = GenerateGunshot();
    m_clips[SoundId::ReloadClick] = GenerateReloadClick();
    m_clips[SoundId::FootstepConcrete] = GenerateFootstep(1.0f);
    m_clips[SoundId::FootstepMetal] = GenerateFootstep(1.6f);
    m_clips[SoundId::FootstepWood] = GenerateFootstep(0.7f);
    m_clips[SoundId::ImpactConcrete] = GenerateImpact(1.0f, 0.3f);
    m_clips[SoundId::ImpactMetal] = GenerateImpact(1.8f, 0.85f);
    m_clips[SoundId::ImpactWood] = GenerateImpact(0.8f, 0.5f);
    m_clips[SoundId::Explosion] = GenerateExplosion();
    m_clips[SoundId::FlashRing] = GenerateFlashRing();
    m_clips[SoundId::PlantBeep] = GenerateBeep(880.0f, 0.15f);
    m_clips[SoundId::DefuseBeep] = GenerateBeep(660.0f, 0.1f);
    m_clips[SoundId::DeathTone] = GenerateDeathTone();
}

const SoundClip& SoundBank::Get(SoundId id) const {
    static const SoundClip kEmpty;
    auto it = m_clips.find(id);
    if (it == m_clips.end()) {
        std::fprintf(stderr, "[SoundBank] Missing clip for sound id %d\n", static_cast<int>(id));
        return kEmpty;
    }
    return it->second;
}

} // namespace Engine
