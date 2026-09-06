#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

namespace Engine {

// Base class for anything placed in a map via an "entity" block — spawn
// points today; pickups, doors, and objective sites in later phases all use
// this same classname + key/value shape, matching how the GoldSrc/Quake map
// format describes non-geometry content.
class Entity {
public:
    virtual ~Entity() = default;

    // Called once every key/value pair from the map file has been loaded,
    // so subclasses can pull out and cache their typed fields instead of
    // re-parsing strings on every access.
    virtual void OnSpawn() {}

    void SetKeyValue(const std::string& key, const std::string& value);
    const std::string& GetClassname() const { return m_classname; }
    void SetClassname(const std::string& classname) { m_classname = classname; }

    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    glm::vec3 GetVector(const std::string& key, const glm::vec3& defaultValue = glm::vec3(0.0f)) const;

private:
    std::string m_classname;
    std::unordered_map<std::string, std::string> m_keyValues;
};

using EntityFactoryFn = std::function<std::unique_ptr<Entity>()>;

// Maps a map file's "classname" string to a constructor for the matching
// C++ type, so the map loader can instantiate the right class without a
// growing if/else chain, and new entity types can register themselves from
// their own .cpp file independently.
class EntityRegistry {
public:
    static EntityRegistry& Instance();

    void Register(const std::string& classname, EntityFactoryFn factory);
    std::unique_ptr<Entity> Create(const std::string& classname) const;

private:
    std::unordered_map<std::string, EntityFactoryFn> m_factories;
};

// Registers classname -> T at static-init time. Declare one of these at
// namespace scope in the .cpp that defines T (see SpawnPointEntity.cpp).
template <typename T>
struct EntityRegistrar {
    explicit EntityRegistrar(const std::string& classname) {
        EntityRegistry::Instance().Register(classname, []() { return std::make_unique<T>(); });
    }
};

#define REGISTER_ENTITY_CLASS(classnameString, TypeName) \
    static ::Engine::EntityRegistrar<TypeName> g_registrar_##TypeName(classnameString)

} // namespace Engine
