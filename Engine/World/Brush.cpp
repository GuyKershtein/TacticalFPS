#include "Brush.h"

#include <cmath>
#include <cstdio>

namespace Engine {

namespace {

constexpr float kClipEpsilon = 0.01f;
constexpr float kFaceQuadSize = 8192.0f;

// Sutherland-Hodgman: keeps the part of `polygon` on or behind `plane`
// (SignedDistance <= epsilon), inserting a new vertex wherever an edge
// crosses the plane.
std::vector<glm::vec3> ClipPolygonByPlane(const std::vector<glm::vec3>& polygon, const Plane& plane) {
    std::vector<glm::vec3> output;
    if (polygon.empty()) return output;

    for (size_t i = 0; i < polygon.size(); ++i) {
        const glm::vec3& current = polygon[i];
        const glm::vec3& next = polygon[(i + 1) % polygon.size()];
        const float distCurrent = plane.SignedDistance(current);
        const float distNext = plane.SignedDistance(next);
        const bool currentInside = distCurrent <= kClipEpsilon;
        const bool nextInside = distNext <= kClipEpsilon;

        if (currentInside) {
            output.push_back(current);
        }
        if (currentInside != nextInside) {
            const float t = distCurrent / (distCurrent - distNext);
            output.push_back(current + t * (next - current));
        }
    }
    return output;
}

} // namespace

void Brush::AddFace(const Plane& plane, const std::string& material) {
    BrushFace face;
    face.plane = plane;
    face.material = material;
    m_faces.push_back(std::move(face));
}

void Brush::BuildGeometry() {
    for (size_t i = 0; i < m_faces.size(); ++i) {
        const Plane& plane = m_faces[i].plane;

        // Build two tangent vectors spanning the plane, with
        // cross(tangent1, tangent2) == normal so the quad below winds CCW
        // as seen from the side the normal points to (the "outside").
        const glm::vec3 up = (std::abs(plane.normal.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 tangent1 = glm::normalize(glm::cross(up, plane.normal));
        const glm::vec3 tangent2 = glm::cross(plane.normal, tangent1);
        const glm::vec3 base = plane.normal * plane.distance;

        std::vector<glm::vec3> polygon = {
            base - tangent1 * kFaceQuadSize - tangent2 * kFaceQuadSize,
            base + tangent1 * kFaceQuadSize - tangent2 * kFaceQuadSize,
            base + tangent1 * kFaceQuadSize + tangent2 * kFaceQuadSize,
            base - tangent1 * kFaceQuadSize + tangent2 * kFaceQuadSize,
        };

        for (size_t j = 0; j < m_faces.size() && !polygon.empty(); ++j) {
            if (j == i) continue;
            polygon = ClipPolygonByPlane(polygon, m_faces[j].plane);
        }

        m_faces[i].polygon = std::move(polygon);
    }
}

void Brush::Triangulate(std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices, const MaterialDatabase& materials) const {
    for (const BrushFace& face : m_faces) {
        if (face.polygon.size() < 3) continue;

        const glm::vec3 color = materials.Get(face.material).color;
        const unsigned int base = static_cast<unsigned int>(outVertices.size());
        for (const glm::vec3& p : face.polygon) {
            outVertices.push_back({p, face.plane.normal, color});
        }
        for (size_t k = 1; k + 1 < face.polygon.size(); ++k) {
            outIndices.push_back(base);
            outIndices.push_back(base + static_cast<unsigned int>(k));
            outIndices.push_back(base + static_cast<unsigned int>(k + 1));
        }
    }
}

Brush Brush::ExpandForHull(const glm::vec3& hullHalfExtents) const {
    Brush expanded;
    for (const BrushFace& face : m_faces) {
        Plane p = face.plane;
        p.distance += glm::dot(hullHalfExtents, glm::abs(p.normal));
        expanded.AddFace(p, face.material);
    }
    expanded.BuildGeometry();
    return expanded;
}

Brush Brush::CreateBox(const glm::vec3& mins, const glm::vec3& maxs, const std::string& material) {
    Brush brush;
    brush.AddFace(Plane{glm::vec3(1.0f, 0.0f, 0.0f), maxs.x}, material);
    brush.AddFace(Plane{glm::vec3(-1.0f, 0.0f, 0.0f), -mins.x}, material);
    brush.AddFace(Plane{glm::vec3(0.0f, 1.0f, 0.0f), maxs.y}, material);
    brush.AddFace(Plane{glm::vec3(0.0f, -1.0f, 0.0f), -mins.y}, material);
    brush.AddFace(Plane{glm::vec3(0.0f, 0.0f, 1.0f), maxs.z}, material);
    brush.AddFace(Plane{glm::vec3(0.0f, 0.0f, -1.0f), -mins.z}, material);
    return brush;
}

} // namespace Engine
