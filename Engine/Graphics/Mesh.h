#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Engine {

// A vertex format of position+normal+color. No UVs/tangents yet — texturing
// is a later Phase 1 milestone; this is the smallest layout that supports
// basic diffuse lighting (Milestone 4) on top of what Milestone 1 had.
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

// Owns a VAO/VBO/EBO triple and knows how to draw itself. Geometry is
// uploaded once at creation (GL_STATIC_DRAW); nothing here is dynamic yet.
class Mesh {
public:
    void Create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    void Destroy();

    void Draw() const;

    static Mesh CreateCube(const glm::vec3& color);
    static Mesh CreatePlane(float width, float depth, const glm::vec3& color);

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    unsigned int m_indexCount = 0;
};

} // namespace Engine
