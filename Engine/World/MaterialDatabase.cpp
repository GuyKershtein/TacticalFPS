#include "MaterialDatabase.h"
#include "../Core/Tokenizer.h"

#include <fstream>
#include <sstream>
#include <cstdio>

namespace Engine {

namespace {
constexpr const char* kContext = "MaterialDatabase";
}

bool MaterialDatabase::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::fprintf(stderr, "[MaterialDatabase] Could not open file: %s\n", path.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string text = ss.str();

    Tokenizer tok(text);
    std::string keyword;
    while (tok.NextToken(keyword)) {
        if (keyword != "material") {
            std::fprintf(stderr, "[MaterialDatabase] Unexpected top-level token '%s'\n", keyword.c_str());
            return false;
        }

        std::string name;
        if (!tok.NextToken(name)) return false;
        if (!ExpectToken(tok, "{", kContext)) return false;

        SurfaceMaterial material;
        material.name = name;

        std::string peek;
        while (tok.PeekToken(peek) && peek != "}") {
            std::string field;
            tok.NextToken(field);
            if (field == "color") {
                if (!ParseVec3(tok, material.color, kContext)) return false;
            } else if (field == "footstep") {
                if (!tok.NextToken(material.footstepSound)) return false;
            } else if (field == "impact") {
                if (!tok.NextToken(material.impactEffect)) return false;
            } else {
                std::fprintf(stderr, "[MaterialDatabase] Unknown field '%s' in material '%s'\n", field.c_str(), name.c_str());
                return false;
            }
        }
        if (!ExpectToken(tok, "}", kContext)) return false;

        m_materials[name] = material;
    }
    return true;
}

const SurfaceMaterial& MaterialDatabase::Get(const std::string& name) const {
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        return it->second;
    }
    std::fprintf(stderr, "[MaterialDatabase] Unregistered material '%s', using fallback\n", name.c_str());
    return m_fallback;
}

} // namespace Engine
