#include "Random.h"

#include <random>

namespace Engine {

namespace {

std::mt19937& GetGenerator() {
    static std::mt19937 generator(std::random_device{}());
    return generator;
}

} // namespace

float Random::Range(float minInclusive, float maxInclusive) {
    std::uniform_real_distribution<float> dist(minInclusive, maxInclusive);
    return dist(GetGenerator());
}

float Random::SignedRange(float maxAbs) {
    return Range(-maxAbs, maxAbs);
}

} // namespace Engine
