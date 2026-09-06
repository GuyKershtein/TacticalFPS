#include "MapLoader.h"
#include "../Core/Tokenizer.h"

#include <fstream>
#include <sstream>
#include <cstdio>

namespace Engine {

namespace {

constexpr const char* kContext = "MapLoader";

// A brush block contains exactly one of:
//   box ( minx miny minz ) ( maxx maxy maxz ) material
//   one or more: plane ( x y z ) ( x y z ) ( x y z ) material
bool ParseBrush(Tokenizer& tok, Brush& outBrush) {
    if (!ExpectToken(tok, "{", kContext)) return false;

    std::string peek;
    while (tok.PeekToken(peek) && peek != "}") {
        std::string faceType;
        tok.NextToken(faceType);

        if (faceType == "box") {
            glm::vec3 mins, maxs;
            std::string material;
            if (!ParseVec3(tok, mins, kContext) || !ParseVec3(tok, maxs, kContext) || !tok.NextToken(material)) return false;
            outBrush = Brush::CreateBox(mins, maxs, material);
        } else if (faceType == "plane") {
            glm::vec3 p0, p1, p2;
            std::string material;
            if (!ParseVec3(tok, p0, kContext) || !ParseVec3(tok, p1, kContext) || !ParseVec3(tok, p2, kContext) || !tok.NextToken(material)) return false;
            outBrush.AddFace(Plane::FromPoints(p0, p1, p2), material);
        } else {
            std::fprintf(stderr, "[MapLoader] Unknown brush face type '%s'\n", faceType.c_str());
            return false;
        }
    }
    return ExpectToken(tok, "}", kContext);
}

bool ParseWorldspawn(Tokenizer& tok, Level& level) {
    if (!ExpectToken(tok, "{", kContext)) return false;

    std::string peek;
    while (tok.PeekToken(peek) && peek != "}") {
        std::string keyword;
        tok.NextToken(keyword);
        if (keyword != "brush") {
            std::fprintf(stderr, "[MapLoader] Unexpected token '%s' in worldspawn\n", keyword.c_str());
            return false;
        }
        Brush brush;
        if (!ParseBrush(tok, brush)) return false;
        brush.BuildGeometry();
        level.worldBrushes.push_back(std::move(brush));
    }
    return ExpectToken(tok, "}", kContext);
}

bool ParseEntity(Tokenizer& tok, Level& level) {
    if (!ExpectToken(tok, "{", kContext)) return false;

    std::vector<std::pair<std::string, std::string>> keyValues;
    std::string peek;
    while (tok.PeekToken(peek) && peek != "}") {
        std::string key, value;
        if (!tok.NextToken(key) || !tok.NextToken(value)) return false;
        keyValues.emplace_back(key, value);
    }
    if (!ExpectToken(tok, "}", kContext)) return false;

    std::string classname;
    for (const auto& kv : keyValues) {
        if (kv.first == "classname") {
            classname = kv.second;
            break;
        }
    }

    std::unique_ptr<Entity> entity = EntityRegistry::Instance().Create(classname);
    if (!entity) {
        // Unrecognized classname: fall back to a generic Entity rather than
        // dropping the map author's data.
        entity = std::make_unique<Entity>();
    }
    entity->SetClassname(classname);
    for (const auto& kv : keyValues) {
        entity->SetKeyValue(kv.first, kv.second);
    }
    entity->OnSpawn();
    level.entities.push_back(std::move(entity));
    return true;
}

} // namespace

bool MapLoader::Load(const std::string& path, Level& outLevel) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::fprintf(stderr, "[MapLoader] Could not open map file: %s\n", path.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string text = ss.str();

    Tokenizer tok(text);
    std::string keyword;
    while (tok.NextToken(keyword)) {
        if (keyword == "worldspawn") {
            if (!ParseWorldspawn(tok, outLevel)) return false;
        } else if (keyword == "entity") {
            if (!ParseEntity(tok, outLevel)) return false;
        } else {
            std::fprintf(stderr, "[MapLoader] Unexpected top-level token '%s'\n", keyword.c_str());
            return false;
        }
    }
    return true;
}

} // namespace Engine
