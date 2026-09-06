#pragma once

#include <vector>
#include <string>
#include "../Math/Plane.h"
#include "../Graphics/Mesh.h"
#include "MaterialDatabase.h"

namespace Engine {

struct BrushFace {
    Plane plane;
    std::string material;
    // The face's bounded polygon (planar, convex, CCW as seen from the side
    // the plane's normal points to). Filled in by Brush::BuildGeometry().
    std::vector<glm::vec3> polygon;
};

// A convex solid defined as the intersection of half-spaces (one per face
// plane) — the same "brush" representation the GoldSrc/Quake map-compiler
// lineage uses for level geometry. Authoring only specifies planes; each
// face's visible polygon is derived by clipping a large quad on that plane
// against every other face's plane (Sutherland-Hodgman).
class Brush {
public:
    void AddFace(const Plane& plane, const std::string& material);

    // Computes every face's bounded polygon from the plane set. Must be
    // called once after all faces are added, before Triangulate() or using
    // this brush for collision.
    void BuildGeometry();

    // Appends this brush's triangles (with per-face normals and a color
    // resolved from `materials`) into shared vertex/index buffers, so an
    // entire level's brushes can be combined into one draw call.
    void Triangulate(std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices, const MaterialDatabase& materials) const;

    const std::vector<BrushFace>& GetFaces() const { return m_faces; }

    // Returns a copy of this brush with every plane pushed outward by the
    // hull's half-extents — the Minkowski sum of the brush with an AABB
    // hull, i.e. the classic GoldSrc trick of colliding a point against an
    // expanded brush instead of a box against the real one. Consumed by the
    // player controller in Milestone 2.
    Brush ExpandForHull(const glm::vec3& hullHalfExtents) const;

    static Brush CreateBox(const glm::vec3& mins, const glm::vec3& maxs, const std::string& material);

private:
    std::vector<BrushFace> m_faces;
};

} // namespace Engine
