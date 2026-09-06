#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Engine {

// Compiles a vertex+fragment shader pair into a GL program and provides
// typed uniform setters. One Shader instance = one GL program.
class Shader {
public:
    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void Destroy();

    void Use() const;

    void SetMat4(const std::string& name, const glm::mat4& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetInt(const std::string& name, int value) const;

    unsigned int GetProgramId() const { return m_programId; }

private:
    bool CompileStage(unsigned int type, const std::string& source, unsigned int& outShader, const std::string& debugName);
    int GetUniformLocation(const std::string& name) const;

    unsigned int m_programId = 0;
};

} // namespace Engine
