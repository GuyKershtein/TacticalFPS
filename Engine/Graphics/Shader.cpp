#include "Shader.h"
#include "GLFunctions.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <glm/gtc/type_ptr.hpp>

namespace Engine {

namespace {

bool ReadFile(const std::string& path, std::string& outContents) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::fprintf(stderr, "[Shader] Could not open file: %s\n", path.c_str());
        return false;
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    outContents = stream.str();
    return true;
}

} // namespace

bool Shader::CompileStage(unsigned int type, const std::string& source, unsigned int& outShader, const std::string& debugName) {
    outShader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(outShader, 1, &src, nullptr);
    glCompileShader(outShader);

    GLint success = 0;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(outShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength > 0 ? logLength : 1);
        glGetShaderInfoLog(outShader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::fprintf(stderr, "[Shader] Compile error in %s:\n%s\n", debugName.c_str(), log.data());
        glDeleteShader(outShader);
        outShader = 0;
        return false;
    }
    return true;
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource, fragmentSource;
    if (!ReadFile(vertexPath, vertexSource)) return false;
    if (!ReadFile(fragmentPath, fragmentSource)) return false;

    unsigned int vertexShader = 0, fragmentShader = 0;
    if (!CompileStage(GL_VERTEX_SHADER, vertexSource, vertexShader, vertexPath)) {
        return false;
    }
    if (!CompileStage(GL_FRAGMENT_SHADER, fragmentSource, fragmentShader, fragmentPath)) {
        glDeleteShader(vertexShader);
        return false;
    }

    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    GLint success = 0;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength > 0 ? logLength : 1);
        glGetProgramInfoLog(m_programId, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::fprintf(stderr, "[Shader] Link error (%s + %s):\n%s\n", vertexPath.c_str(), fragmentPath.c_str(), log.data());
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_programId);
        m_programId = 0;
        return false;
    }

    // Once linked, the shader objects are no longer needed as separate objects.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

void Shader::Destroy() {
    if (m_programId) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
}

void Shader::Use() const {
    glUseProgram(m_programId);
}

int Shader::GetUniformLocation(const std::string& name) const {
    return glGetUniformLocation(m_programId, name.c_str());
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetFloat(const std::string& name, float value) const {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetInt(const std::string& name, int value) const {
    glUniform1i(GetUniformLocation(name), value);
}

} // namespace Engine
