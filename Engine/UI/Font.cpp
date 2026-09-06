#include "Font.h"
#include "../Graphics/GLFunctions.h"

#include "stb_truetype.h"

#include <cstdio>
#include <fstream>
#include <vector>

namespace Engine {

namespace {

bool ReadWholeFile(const std::string& path, std::vector<unsigned char>& outBytes) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    const std::streamsize size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0, std::ios::beg);
    outBytes.resize(static_cast<size_t>(size));
    return static_cast<bool>(file.read(reinterpret_cast<char*>(outBytes.data()), size));
}

} // namespace

bool Font::LoadFromFile(const std::string& ttfPath, float pixelHeight) {
    std::vector<unsigned char> ttfBuffer;
    if (!ReadWholeFile(ttfPath, ttfBuffer)) {
        std::fprintf(stderr, "[Font] Could not read font file: %s\n", ttfPath.c_str());
        return false;
    }

    std::vector<unsigned char> bitmap(kAtlasSize * kAtlasSize);
    m_bakedCharData.resize(kCharCount * sizeof(stbtt_bakedchar));
    auto* bakedChars = reinterpret_cast<stbtt_bakedchar*>(m_bakedCharData.data());

    const int result = stbtt_BakeFontBitmap(
        ttfBuffer.data(), 0, pixelHeight,
        bitmap.data(), kAtlasSize, kAtlasSize,
        kFirstChar, kCharCount, bakedChars);
    if (result <= 0) {
        std::fprintf(stderr, "[Font] Atlas too small for %s at size %.0f (stb_truetype returned %d)\n",
            ttfPath.c_str(), pixelHeight, result);
        return false;
    }
    m_pixelHeight = pixelHeight;

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, kAtlasSize, kAtlasSize, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Font::Destroy() {
    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
}

void Font::GetGlyphQuad(char c, float& penX, float& penY, GlyphQuad& outQuad) const {
    const int index = static_cast<int>(static_cast<unsigned char>(c)) - kFirstChar;
    if (index < 0 || index >= kCharCount) {
        // Unsupported byte (control chars, extended ASCII) — advance by a
        // space's width rather than drawing garbage or stalling layout.
        const int spaceIndex = static_cast<int>(' ') - kFirstChar;
        const auto* baked = reinterpret_cast<const stbtt_bakedchar*>(m_bakedCharData.data());
        penX += baked[spaceIndex].xadvance;
        return;
    }

    const auto* baked = reinterpret_cast<const stbtt_bakedchar*>(m_bakedCharData.data());
    stbtt_aligned_quad quad{};
    stbtt_GetBakedQuad(baked, kAtlasSize, kAtlasSize, index, &penX, &penY, &quad, 1);
    outQuad.x0 = quad.x0; outQuad.y0 = quad.y0; outQuad.x1 = quad.x1; outQuad.y1 = quad.y1;
    outQuad.s0 = quad.s0; outQuad.t0 = quad.t0; outQuad.s1 = quad.s1; outQuad.t1 = quad.t1;
}

float Font::MeasureWidth(const std::string& text) const {
    float penX = 0.0f;
    float penY = 0.0f;
    for (const char c : text) {
        GlyphQuad quad;
        GetGlyphQuad(c, penX, penY, quad);
    }
    return penX;
}

} // namespace Engine
