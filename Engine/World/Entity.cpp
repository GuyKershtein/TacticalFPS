#include "Entity.h"

#include <cstdlib>
#include <sstream>

namespace Engine {

void Entity::SetKeyValue(const std::string& key, const std::string& value) {
    m_keyValues[key] = value;
}

std::string Entity::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_keyValues.find(key);
    return it != m_keyValues.end() ? it->second : defaultValue;
}

float Entity::GetFloat(const std::string& key, float defaultValue) const {
    auto it = m_keyValues.find(key);
    if (it == m_keyValues.end()) return defaultValue;
    return std::strtof(it->second.c_str(), nullptr);
}

glm::vec3 Entity::GetVector(const std::string& key, const glm::vec3& defaultValue) const {
    auto it = m_keyValues.find(key);
    if (it == m_keyValues.end()) return defaultValue;
    std::istringstream stream(it->second);
    glm::vec3 result = defaultValue;
    stream >> result.x >> result.y >> result.z;
    return result;
}

EntityRegistry& EntityRegistry::Instance() {
    static EntityRegistry instance;
    return instance;
}

void EntityRegistry::Register(const std::string& classname, EntityFactoryFn factory) {
    m_factories[classname] = std::move(factory);
}

std::unique_ptr<Entity> EntityRegistry::Create(const std::string& classname) const {
    auto it = m_factories.find(classname);
    if (it != m_factories.end()) {
        return it->second();
    }
    return nullptr; // unrecognized classname; MapLoader falls back to a generic Entity so key/value data is never dropped
}

} // namespace Engine
