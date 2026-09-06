#include "CollisionQuery.h"

namespace Engine {

namespace {

struct BrushHit {
    bool hit = false;
    float fraction = 1.0f;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    std::string material;
};

// Sweeps a point through one convex brush (a set of half-space planes).
// Standard convex-polytope sweep test: for each plane, find where the
// segment crosses it; the move enters the brush's solid volume at the
// *latest* of those entry crossings and would leave at the *earliest* exit
// crossing. If the latest entry happens before the earliest exit, the move
// actually reaches solid space and that entry point is the collision.
// A point exactly on a brush's boundary (SignedDistance == 0, e.g. a player
// resting flush on a floor) must count as "outside" for the startedOutside
// check below, not "embedded" — otherwise a mover sitting exactly on a
// surface would look identical to one truly stuck inside solid geometry,
// and ground detection (a probe starting right at the surface) would
// silently stop registering hits.
constexpr float kOnPlaneEpsilon = 0.0001f;

BrushHit TraceSingleBrush(const glm::vec3& start, const glm::vec3& end, const Brush& brush) {
    float enterFraction = 0.0f;
    float leaveFraction = 1.0f;
    bool startedOutside = false;
    bool everCrossedIntoSolid = false;
    glm::vec3 enterNormal(0.0f, 1.0f, 0.0f);
    std::string enterMaterial;

    for (const BrushFace& face : brush.GetFaces()) {
        const float d1 = face.plane.SignedDistance(start);
        const float d2 = face.plane.SignedDistance(end);

        if (d1 > -kOnPlaneEpsilon) startedOutside = true;

        if (d1 > 0.0f && d2 > 0.0f) {
            return {}; // outside this plane for the entire move: never enters this brush at all
        }
        if (d1 <= 0.0f && d2 <= 0.0f) {
            continue; // this plane never constrains the move
        }

        const float f = d1 / (d1 - d2);
        if (d1 > d2) {
            // Crossing from outside to inside this plane: a candidate entry.
            if (f > enterFraction) {
                enterFraction = f;
                enterNormal = face.plane.normal;
                enterMaterial = face.material;
                everCrossedIntoSolid = true;
            }
        } else {
            // Crossing from inside to outside this plane: a candidate exit.
            if (f < leaveFraction) {
                leaveFraction = f;
            }
        }
    }

    // If the start point was never outside any plane, it's already fully
    // embedded in this brush's solid volume. Rather than reporting a
    // zero-distance hit (which would freeze the mover in place), treat this
    // brush as non-blocking for this move — an embedded start should only
    // happen from float error at a shared seam between adjacent brushes.
    if (!startedOutside) {
        return {};
    }

    BrushHit result;
    if (everCrossedIntoSolid && enterFraction <= leaveFraction && enterFraction < 1.0f) {
        result.hit = true;
        result.fraction = enterFraction;
        result.normal = enterNormal;
        result.material = enterMaterial;
    }
    return result;
}

// Checks whether `position` is resting on (or within maxDistance above) one
// particular brush: some upward-facing plane must be within [-epsilon,
// maxDistance] of the point, and every *other* plane of the brush must have
// the point on or behind it (i.e. the point sits within that face's extent,
// not off to the side of the brush entirely).
bool CheckBrushGroundContact(const glm::vec3& position, float maxDistance, const Brush& brush, float& outDistance, glm::vec3& outNormal) {
    const std::vector<BrushFace>& faces = brush.GetFaces();

    for (const BrushFace& candidate : faces) {
        if (candidate.plane.normal.y <= 0.7f) continue; // not upward-facing enough to stand on

        const float candidateDist = candidate.plane.SignedDistance(position);
        if (candidateDist < -kOnPlaneEpsilon || candidateDist > maxDistance) continue;

        bool withinFootprint = true;
        for (const BrushFace& other : faces) {
            if (&other == &candidate) continue;
            if (other.plane.SignedDistance(position) > kOnPlaneEpsilon) {
                withinFootprint = false;
                break;
            }
        }

        if (withinFootprint) {
            outDistance = candidateDist;
            outNormal = candidate.plane.normal;
            return true;
        }
    }
    return false;
}

} // namespace

TraceResult TraceMove(const glm::vec3& start, const glm::vec3& end, const std::vector<Brush>& brushes) {
    TraceResult best;
    best.fraction = 1.0f;
    best.endPosition = end;

    for (const Brush& brush : brushes) {
        const BrushHit hit = TraceSingleBrush(start, end, brush);
        if (hit.hit && hit.fraction < best.fraction) {
            best.hit = true;
            best.fraction = hit.fraction;
            best.planeNormal = hit.normal;
            best.material = hit.material;
        }
    }

    best.endPosition = start + (end - start) * best.fraction;
    return best;
}

glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal, float overbounce) {
    const float backoff = glm::dot(velocity, normal) * overbounce;
    return velocity - normal * backoff;
}

void SlideMove(glm::vec3& position, glm::vec3& velocity, float deltaTime, const std::vector<Brush>& brushes) {
    constexpr int kMaxBumps = 4;
    // Small pull-back off the hit surface so the next bump's trace doesn't
    // immediately re-report a zero-distance hit from float error.
    constexpr float kSurfaceEpsilon = 0.03125f;

    float timeLeft = deltaTime;
    std::vector<glm::vec3> planesTouchedThisMove;

    for (int bump = 0; bump < kMaxBumps && timeLeft > 0.0f; ++bump) {
        const glm::vec3 end = position + velocity * timeLeft;
        const TraceResult trace = TraceMove(position, end, brushes);

        position = trace.endPosition;
        if (!trace.hit) {
            break; // reached the intended destination with nothing in the way
        }

        position += trace.planeNormal * kSurfaceEpsilon;
        timeLeft -= timeLeft * trace.fraction;
        planesTouchedThisMove.push_back(trace.planeNormal);

        // Re-clip velocity against every plane touched so far this move, so
        // running into a corner (two planes at once) slides along the seam
        // instead of getting stuck on whichever plane was hit first.
        glm::vec3 slid = velocity;
        for (const glm::vec3& normal : planesTouchedThisMove) {
            slid = ClipVelocity(slid, normal, 1.0f);
        }
        velocity = slid;

        if (glm::dot(velocity, velocity) < 0.001f) {
            velocity = glm::vec3(0.0f);
            break;
        }
    }
}

TraceResult CheckGroundContact(const glm::vec3& position, float maxDistance, const std::vector<Brush>& brushes) {
    TraceResult best;
    best.fraction = 1.0f;

    float bestDistance = maxDistance + 1.0f;
    for (const Brush& brush : brushes) {
        float distance;
        glm::vec3 normal;
        if (CheckBrushGroundContact(position, maxDistance, brush, distance, normal) && distance < bestDistance) {
            bestDistance = distance;
            best.hit = true;
            best.planeNormal = normal;
        }
    }
    return best;
}

} // namespace Engine
