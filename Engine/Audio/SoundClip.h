#pragma once

#include <vector>
#include <cstdint>

namespace Engine {

// Raw mono PCM audio data (16-bit signed, engine-generated). Mono because
// every clip is played back through 3D positional spatialization, which
// derives its own stereo image from listener/source position — a
// pre-mixed stereo source would fight that.
struct SoundClip {
    std::vector<int16_t> samples;
    uint32_t sampleRate = 44100;
};

} // namespace Engine
