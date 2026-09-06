#include "ProceduralSoundGenerator.h"
#include "../Math/Random.h"

#include <cmath>
#include <algorithm>

namespace Engine {
namespace ProceduralSound {

namespace {

constexpr uint32_t kSampleRate = 44100;

// A one-pole low-pass filter: smaller alpha = duller/rounder, larger alpha
// = brighter/sharper. Turns raw white noise into something that reads as
// a "thud" (low alpha) vs a "crack" (high alpha) without needing real
// recorded material.
void ApplyLowPass(std::vector<float>& samples, float alpha) {
    float previous = 0.0f;
    for (float& sample : samples) {
        previous += alpha * (sample - previous);
        sample = previous;
    }
}

std::vector<float> GenerateNoise(float durationSeconds) {
    const size_t count = static_cast<size_t>(durationSeconds * kSampleRate);
    std::vector<float> samples(count);
    for (float& sample : samples) {
        sample = Engine::Random::Range(-1.0f, 1.0f);
    }
    return samples;
}

// Exponential decay envelope: starts at 1, decays to ~0 by the end.
void ApplyExponentialDecay(std::vector<float>& samples, float decaySharpness) {
    const size_t count = samples.size();
    for (size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(count);
        samples[i] *= std::exp(-decaySharpness * t);
    }
}

std::vector<float> GenerateSineSweep(float freqStart, float freqEnd, float durationSeconds) {
    const size_t count = static_cast<size_t>(durationSeconds * kSampleRate);
    std::vector<float> samples(count);
    double phase = 0.0;
    for (size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(count);
        const float freq = freqStart + (freqEnd - freqStart) * t;
        phase += 2.0 * 3.14159265358979 * freq / kSampleRate;
        samples[i] = static_cast<float>(std::sin(phase));
    }
    return samples;
}

SoundClip ToClip(std::vector<float> samples, float masterVolume = 0.7f) {
    SoundClip clip;
    clip.sampleRate = kSampleRate;
    clip.samples.resize(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        const float clamped = std::clamp(samples[i] * masterVolume, -1.0f, 1.0f);
        clip.samples[i] = static_cast<int16_t>(clamped * 32767.0f);
    }
    return clip;
}

void Mix(std::vector<float>& dest, const std::vector<float>& src, float gain = 1.0f) {
    if (dest.size() < src.size()) dest.resize(src.size(), 0.0f);
    for (size_t i = 0; i < src.size(); ++i) {
        dest[i] += src[i] * gain;
    }
}

} // namespace

SoundClip GenerateGunshot() {
    auto crack = GenerateNoise(0.12f);
    ApplyLowPass(crack, 0.9f); // bright, sharp
    ApplyExponentialDecay(crack, 14.0f);

    auto body = GenerateNoise(0.18f);
    ApplyLowPass(body, 0.35f); // duller low-end thump under the crack
    ApplyExponentialDecay(body, 9.0f);

    Mix(crack, body, 0.5f);
    return ToClip(crack, 0.8f);
}

SoundClip GenerateReloadClick() {
    auto click = GenerateNoise(0.04f);
    ApplyLowPass(click, 0.95f);
    ApplyExponentialDecay(click, 25.0f);
    return ToClip(click, 0.5f);
}

SoundClip GenerateFootstep(float pitch) {
    auto thud = GenerateNoise(0.10f * (1.0f / std::max(pitch, 0.1f)));
    ApplyLowPass(thud, 0.25f * pitch);
    ApplyExponentialDecay(thud, 16.0f);
    return ToClip(thud, 0.35f);
}

SoundClip GenerateImpact(float pitch, float brightness) {
    auto noise = GenerateNoise(0.15f);
    ApplyLowPass(noise, std::clamp(brightness, 0.05f, 0.98f));
    ApplyExponentialDecay(noise, 10.0f);

    // A short pitched component gives metal impacts a "ring" that pure
    // filtered noise can't produce on its own.
    if (pitch > 1.0f) {
        auto ring = GenerateSineSweep(1200.0f * pitch, 600.0f * pitch, 0.2f);
        ApplyExponentialDecay(ring, 6.0f);
        Mix(noise, ring, 0.4f);
    }
    return ToClip(noise, 0.6f);
}

SoundClip GenerateExplosion() {
    auto blast = GenerateNoise(1.1f);
    ApplyLowPass(blast, 0.5f);
    ApplyExponentialDecay(blast, 3.0f);

    auto sub = GenerateSineSweep(120.0f, 30.0f, 0.9f);
    ApplyExponentialDecay(sub, 3.5f);

    Mix(blast, sub, 0.6f);
    return ToClip(blast, 0.9f);
}

SoundClip GenerateFlashRing() {
    // A sustained high tone — the "ringing ears" cliche, which happens to
    // be exactly the right sound for a flashbang and needs no noise layer.
    auto ring = GenerateSineSweep(4200.0f, 3800.0f, 1.6f);
    ApplyExponentialDecay(ring, 0.6f);
    return ToClip(ring, 0.35f);
}

SoundClip GenerateBeep(float frequencyHz, float durationSeconds) {
    auto tone = GenerateSineSweep(frequencyHz, frequencyHz, durationSeconds);
    ApplyExponentialDecay(tone, 2.0f);
    return ToClip(tone, 0.5f);
}

SoundClip GenerateDeathTone() {
    auto tone = GenerateSineSweep(440.0f, 110.0f, 0.6f);
    ApplyExponentialDecay(tone, 2.5f);
    return ToClip(tone, 0.5f);
}

} // namespace ProceduralSound
} // namespace Engine
