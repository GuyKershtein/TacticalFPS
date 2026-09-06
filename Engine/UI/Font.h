#pragma once

#include <string>
#include <vector>

namespace Engine {

// A single baked-bitmap quad for one glyph: position in pixels (top-left
// origin, matching Window/InputManager's coordinate convention) and UV in
// the font atlas texture. Mirrors stb_truetype's stbtt_aligned_quad, but
// this header doesn't include stb_truetype.h itself — same "keep the
// third-party header out of the public interface" rule AudioSystem.h
// follows for miniaudio's ma_engine.
struct GlyphQuad {
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    float s0 = 0.0f, t0 = 0.0f, s1 = 0.0f, t1 = 0.0f;
};

// Rasterizes a TrueType font into a single-channel bitmap atlas (via
// stb_truetype) covering printable ASCII, and answers per-glyph quad
// queries against it. Loads directly from an OS-installed font file
// (Consolas, shipped with every Windows install) rather than bundling a
// font asset or fetching one — zero extra network/licensing dependency,
// and a real, permanent implementation rather than a placeholder.
class Font {
public:
    bool LoadFromFile(const std::string& ttfPath, float pixelHeight);
    void Destroy();

    // Advances (penX, penY) past the glyph and fills outQuad. Call once per
    // character in sequence to lay out a string; ignores characters outside
    // the baked range (falls back to the space advance).
    void GetGlyphQuad(char c, float& penX, float& penY, GlyphQuad& outQuad) const;

    float MeasureWidth(const std::string& text) const;
    float GetPixelHeight() const { return m_pixelHeight; }
    unsigned int GetTextureId() const { return m_textureId; }
    bool IsLoaded() const { return m_textureId != 0; }

private:
    static constexpr int kAtlasSize = 512;
    static constexpr int kFirstChar = 32;  // ' '
    static constexpr int kCharCount = 95;  // through '~'

    // Opaque storage for kCharCount stbtt_bakedchar entries — sized and
    // reinterpreted in the .cpp so this header stays free of stb_truetype's
    // types, same reasoning as GlyphQuad above.
    std::vector<unsigned char> m_bakedCharData;

    unsigned int m_textureId = 0;
    float m_pixelHeight = 0.0f;
};

} // namespace Engine
