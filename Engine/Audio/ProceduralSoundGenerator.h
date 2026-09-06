#pragma once

#include "SoundClip.h"

namespace Engine {
namespace ProceduralSound {

// No real audio assets exist (or can be authored through this project's
// toolchain) — these synthesize original placeholder sounds at startup
// instead of shipping silence. Each is a simple combination of filtered
// noise bursts and tone sweeps with an amplitude envelope; replacing them
// with real recorded/designed audio later is a SoundBank-only change,
// nothing downstream needs to know a clip was synthesized.
SoundClip GenerateGunshot();
SoundClip GenerateReloadClick();
SoundClip GenerateFootstep(float pitch);           // pitch varies the "surface" feel
SoundClip GenerateImpact(float pitch, float brightness);
SoundClip GenerateExplosion();
SoundClip GenerateFlashRing();
SoundClip GenerateBeep(float frequencyHz, float durationSeconds);
SoundClip GenerateDeathTone();

} // namespace ProceduralSound
} // namespace Engine
