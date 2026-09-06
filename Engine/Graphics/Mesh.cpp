#include "Mesh.h"
#include "GLFunctions.h"

namespace Engine {

void Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    m_indexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    // layout(location = 0) vec3 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

    // layout(location = 1) vec3 normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));

    // layout(location = 2) vec3 color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));

    glBindVertexArray(0);
}

void Mesh::Destroy() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_ebo = m_vbo = m_vao = 0;
    m_indexCount = 0;
}

void Mesh::Draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Mesh Mesh::CreateCube(const glm::vec3& color) {
    static const glm::vec3 faceNormals[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    static const float positions[6][4][3] = {
        // +X
        {{0.5f,-0.5f,-0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,0.5f,-0.5f}},
        // -X
        {{-0.5f,-0.5f,0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{-0.5f,0.5f,0.5f}},
        // +Y
        {{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}},
        // -Y
        {{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f}},
        // +Z
        {{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}},
        // -Z
        {{0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f}},
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(24);
    indices.reserve(36);

    for (int face = 0; face < 6; ++face) {
        const unsigned int base = static_cast<unsigned int>(vertices.size());
        for (int v = 0; v < 4; ++v) {
            const float* p = positions[face][v];
            vertices.push_back({glm::vec3(p[0], p[1], p[2]), faceNormals[face], color});
        }
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    Mesh mesh;
    mesh.Create(vertices, indices);
    return mesh;
}

Mesh Mesh::CreatePlane(float width, float depth, const glm::vec3& color) {
    const float hw = width * 0.5f;
    const float hd = depth * 0.5f;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    std::vector<Vertex> vertices = {
        {{-hw, 0.0f, -hd}, up, color},
        {{ hw, 0.0f, -hd}, up, color},
        {{ hw, 0.0f,  hd}, up, color},
        {{-hw, 0.0f,  hd}, up, color},
    };
    // Wound so the triangle is CCW (front-facing) as seen by a camera above
    // looking down at the +Y side of the plane, matching our GL_CCW/GL_BACK
    // culling setup.
    std::vector<unsigned int> indices = {0, 2, 1, 0, 3, 2};

    Mesh mesh;
    mesh.Create(vertices, indices);
    return mesh;
}

} // namespace Engine
