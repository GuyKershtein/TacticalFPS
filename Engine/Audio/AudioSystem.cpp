#include "AudioSystem.h"
#include "miniaudio.h"

#include <cstdio>
#include <algorithm>

namespace Engine {

struct AudioSystem::ActiveSound {
    ma_audio_buffer buffer{};
    ma_sound sound{};
    bool soundInitialized = false;

    ~ActiveSound() {
        // miniaudio objects aren't RAII on their own — every _init needs a
        // matching _uninit, which this destructor is the one place that
        // guarantees runs, whether reaped by Update() or torn down by
        // Shutdown() clearing the vector.
        if (soundInitialized) {
            ma_sound_uninit(&sound);
        }
        ma_audio_buffer_uninit(&buffer);
    }
};

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Init() {
    m_engine = new ma_engine();
    const ma_result result = ma_engine_init(nullptr, m_engine);
    if (result != MA_SUCCESS) {
        std::fprintf(stderr, "[AudioSystem] ma_engine_init failed (%d)\n", static_cast<int>(result));
        delete m_engine;
        m_engine = nullptr;
        return false;
    }
    return true;
}

void AudioSystem::Shutdown() {
    m_activeSounds.clear(); // ActiveSound's destructor (below) tears down each miniaudio object
    if (m_engine) {
        ma_engine_uninit(m_engine);
        delete m_engine;
        m_engine = nullptr;
    }
}

void AudioSystem::SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
    if (!m_engine) return;
    ma_engine_listener_set_position(m_engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(m_engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(m_engine, 0, up.x, up.y, up.z);
}

void AudioSystem::PlaySound3D(const SoundClip& clip, const glm::vec3& position, float volume) {
    if (!m_engine || clip.samples.empty()) return;

    auto active = std::make_unique<ActiveSound>();

    ma_audio_buffer_config config = ma_audio_buffer_config_init(
        ma_format_s16, 1, clip.samples.size(), clip.samples.data(), nullptr);
    config.sampleRate = clip.sampleRate;

    if (ma_audio_buffer_init(&config, &active->buffer) != MA_SUCCESS) {
        std::fprintf(stderr, "[AudioSystem] ma_audio_buffer_init failed\n");
        return;
    }

    if (ma_sound_init_from_data_source(m_engine, &active->buffer, MA_SOUND_FLAG_NO_PITCH, nullptr, &active->sound) != MA_SUCCESS) {
        std::fprintf(stderr, "[AudioSystem] ma_sound_init_from_data_source failed\n");
        ma_audio_buffer_uninit(&active->buffer);
        return;
    }
    active->soundInitialized = true;

    ma_sound_set_position(&active->sound, position.x, position.y, position.z);
    ma_sound_set_volume(&active->sound, volume);
    ma_sound_set_spatialization_enabled(&active->sound, MA_TRUE);
    ma_sound_start(&active->sound);

    m_activeSounds.push_back(std::move(active));
}

void AudioSystem::Update() {
    m_activeSounds.erase(
        std::remove_if(m_activeSounds.begin(), m_activeSounds.end(),
            [](const std::unique_ptr<ActiveSound>& active) {
                return active->soundInitialized && ma_sound_at_end(const_cast<ma_sound*>(&active->sound));
            }),
        m_activeSounds.end());
}

} // namespace Engine
